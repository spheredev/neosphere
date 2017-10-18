using System;
using System.Collections.Generic;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

namespace Sphere.Gdk.Debugger
{
    class StackFrameInfo
    {
        public StackFrameInfo(string functionName, string fileName, int lineNumber)
        {
            this.FunctionName = functionName;
            this.FileName = fileName;
            this.LineNumber = lineNumber;
        }

        public string FunctionName { get; set; }
        public string FileName     { get; set; }
        public int    LineNumber   { get; set; }
    }

    class ThrowEventArgs : EventArgs
    {
        public ThrowEventArgs(string message, string filename, int lineNumber, bool isFatal)
        {
            Message = message;
            FileName = filename;
            LineNumber = lineNumber;
            IsFatal = isFatal;
        }

        /// <summary>
        /// Gets the filename of the script throwing the error.
        /// </summary>
        public string FileName { get; private set; }

        /// <summary>
        /// Gets whether the error was fatal, i.e. unhandled.
        /// </summary>
        public bool IsFatal { get; private set; }

        /// <summary>
        /// Gets the line number where the error was thrown.
        /// </summary>
        public int LineNumber { get; private set; }

        /// <summary>
        /// Gets the string representation of the thrown value.
        /// </summary>
        public string Message { get; private set; }

    }

    class TraceEventArgs : EventArgs
    {
        public TraceEventArgs(string text)
        {
            Text = text;
        }

        public string Text { get; private set; }
    }
    
    /// <summary>
    /// Allows control of an SSj/Ki debug target over TCP.
    /// </summary>
    class Inferior : IDisposable
    {
        private TcpClient tcp = new TcpClient() { NoDelay = true };
        private Thread messenger = null;
        private object replyLock = new object();
        private Queue<KiMessage> requests = new Queue<KiMessage>();
        private Dictionary<KiMessage, KiMessage> replies = new Dictionary<KiMessage, KiMessage>();
        private int protocolVersion;

        /// <summary>
        /// Constructs an Inferior to control a Duktape debuggee.
        /// </summary>
        public Inferior()
        {
        }

        ~Inferior()
        {
            Dispose();
        }

        /// <summary>
        /// Releases all resources used by the Inferior.
        /// </summary>
        public void Dispose()
        {
            messenger?.Abort();
            Attached = null;
            Detached = null;
            Print = null;
            Status = null;
            Throw = null;
            tcp.Close();
        }

        /// <summary>
        /// Fires when the debugger is attached to a target.
        /// </summary>
        public event EventHandler Attached;

        /// <summary>
        /// Fires when the debugger is detached from a target.
        /// </summary>
        public event EventHandler Detached;

        /// <summary>
        /// Fires when a script calls print().
        /// </summary>
        public event EventHandler<TraceEventArgs> Print;

        /// <summary>
        /// Fires when execution status (code position, etc.) has changed.
        /// </summary>
        public event EventHandler Status;

        /// <summary>
        /// Fires when an error is thrown by JavaScript code.
        /// </summary>
        public event EventHandler<ThrowEventArgs> Throw;

        /// <summary>
        /// Gets the target's application identification string.
        /// </summary>
        public string TargetID { get; private set; }

        /// <summary>
        /// Gets the filename of the last breakpoint hit.
        /// </summary>
        public string FileName { get; private set; }

        /// <summary>
        /// Gets the line number of the last breakpoint hit.
        /// </summary>
        public int LineNumber { get; private set; }

        /// <summary>
        /// Gets whether the target is actively executing code.
        /// </summary>
        public bool Running { get; private set; }

        public async Task Connect(string hostname, int port)
        {
            // connect to Duktape debug server
            await tcp.ConnectAsync(hostname, port);
            string line = "";
            await Task.Run(() =>
            {
                byte[] buffer = new byte[1];
                while (buffer[0] != '\n') {
                    tcp.Client.ReceiveAll(buffer);
                    line += (char)buffer[0];
                }
            });
            string[] handshake = line.Trim().Split(new[] { ' ' }, 3);
            if (handshake[0] != "SSj/Ki")
                throw new NotSupportedException("SSj/Ki handshake failed, unsupported protocol");
            protocolVersion = int.Parse(handshake[1].Substring(1));
            if (protocolVersion < 1 || protocolVersion > 1)
                throw new NotSupportedException("Unsupported SSj/Ki protocol version!");

            TargetID = handshake[2];

            // start the communication thread
            messenger = new Thread(ProcessMessages) { IsBackground = true };
            messenger.Start();
            await DoRequest(KiTag.REQ, KiRequest.SetWatermark, "ssj blue", 0, 255, 255);
            Attached?.Invoke(this, EventArgs.Empty);
        }

        /// <summary>
        /// Sets a breakpoint. Execution will pause automatically if the breakpoint is hit.
        /// </summary>
        /// <param name="filename">The filename in which to place the breakpoint.</param>
        /// <param name="lineNumber">The line number of the breakpoint.</param>
        /// <returns>The index assigned to the breakpoint by Duktape.</returns>
        public async Task<int> AddBreak(string filename, int lineNumber)
        {
            var reply = await DoRequest(KiTag.REQ, KiRequest.AddBreakpoint, filename, lineNumber);
            return (int)reply[1];
        }

        /// <summary>
        /// Clears the breakpoint with the specified index.
        /// </summary>
        /// <param name="index">The index of the breakpoint to clear, as returned by AddBreak.</param>
        /// <returns></returns>
        public async Task DelBreak(int index)
        {
            await DoRequest(KiTag.REQ, KiRequest.DelBreakpoint, index);
        }
        
        /// <summary>
        /// Requests that Duktape end the debug session.
        /// </summary>
        /// <returns></returns>
        public async Task Detach()
        {
            if (messenger == null)
                return;
            await DoRequest(KiTag.REQ, KiRequest.Detach);
            await Task.Run(() => messenger.Join());
            tcp.Client.Disconnect(true);
            Detached?.Invoke(this, EventArgs.Empty);
        }

        /// <summary>
        /// Evaluates a JS expression and returns the result.
        /// </summary>
        /// <param name="expression">The expression or statement to evaluate.</param>
        /// <param name="stackOffset">The point in the stack to do the eval. 0 is active call, 1 the caller, etc..</param>
        /// <returns>The value produced by the expression.</returns>
        public async Task<KiAtom> Eval(string expression, int stackOffset = 0)
        {
            var reply = await DoRequest(KiTag.REQ, KiRequest.Eval, stackOffset, expression);
            return reply[2];
        }

        /// <summary>
        /// Gets a list of function calls currently on the stack.
        /// </summary>
        /// <returns>
        /// An array of StackFrameInfos naming the function, filename and current line number
        /// of each function call on the stack, from top to the bottom.
        /// </returns>
        public async Task<StackFrameInfo[]> GetCallStack()
        {
            var reply = await DoRequest(KiTag.REQ, KiRequest.InspectStack);
            var stack = new List<StackFrameInfo>();
            int count = (reply.Length - 1) / 4;
            for (int i = 0; i < count; ++i) {
                string filename = (string)reply[1 + i * 4];
                string functionName = (string)reply[2 + i * 4];
                int lineNumber = (int)reply[3 + i * 4];
                int column = (int)reply[4 + i * 4];
                stack.Add(new StackFrameInfo(functionName, filename, lineNumber));
            }
            return stack.ToArray();
        }

        /// <summary>
        /// Gets a list of local values and their values. Note that objects
        /// are not evaluated and are listed simply as "{ obj: 'ClassName' }".
        /// </summary>
        /// <param name="stackOffset">The stack frame to get locals for, 0 being the current activation.</param>
        /// <returns></returns>
        public async Task<IReadOnlyDictionary<string, KiAtom>> GetLocals(int stackOffset = 0)
        {
            var reply = await DoRequest(KiTag.REQ, KiRequest.InspectLocals, stackOffset);
            var vars = new Dictionary<string, KiAtom>();
            int count = (reply.Length - 1) / 3;
            for (int i = 0; i < count; ++i) {
                string name = (string)reply[1 + i * 3];
                KiAtom value = reply[3 + i * 3];
                vars[name] = value;
            }
            return vars;
        }

        public async Task<Dictionary<string, PropDesc>> GetObjPropDescRange(Handle handle, int start, int end)
        {
            var reply = await DoRequest(KiTag.REQ, KiRequest.InspectObject, handle, start, end);
            var props = new Dictionary<string, PropDesc>();
            int count = (reply.Length - 1) / 2;
            int i = 1;
            while (i < reply.Length) {
                PropFlags flags = (PropFlags)(int)reply[i++];
                string name = reply[i++].ToString();
                if (flags.HasFlag(PropFlags.Accessor)) {
                    KiAtom getter = reply[i++];
                    KiAtom setter = reply[i++];
                    PropDesc propValue = new PropDesc(getter, setter, flags);
                    if (!flags.HasFlag(PropFlags.Internal))
                        props.Add(name, propValue);
                }
                else {
                    KiAtom value = reply[i++];
                    PropDesc propValue = new PropDesc(value, flags);
                    if (!flags.HasFlag(PropFlags.Internal))
                        props.Add(name, propValue);
                }
            }
            return props;
        }

        public async Task<string> GetSource(string fileName)
        {
            var reply = await DoRequest(KiTag.REQ, KiRequest.GetSource, fileName);
            return reply[0].Tag != KiTag.ERR ? (string)reply[1] : null;
        }

        /// <summary>
        /// Gets a list of currently set breakpoints.
        /// </summary>
        /// <returns>
        /// An array of 2-tuples specifying the location of each breakpoint
        /// as a filename/line number pair
        /// </returns>
        public async Task<Tuple<string, int>[]> ListBreak()
        {
            var reply = await DoRequest(KiTag.REQ, KiRequest.GetBreakpoints);
            var count = (reply.Length - 1) / 2;
            List<Tuple<string, int>> list = new List<Tuple<string, int>>();
            for (int i = 0; i < count; ++i) {
                var breakpoint = Tuple.Create(
                    (string)reply[1 + i * 2], 
                    (int)reply[2 + i * 2]);
                list.Add(breakpoint);
            }
            return list.ToArray();
        }

        /// <summary>
        /// Requests Duktape to pause execution and break into the debugger.
        /// This may take a second or so to register.
        /// </summary>
        /// <returns></returns>
        public async Task Pause()
        {
            await DoRequest(KiTag.REQ, KiRequest.Pause);
        }

        /// <summary>
        /// Resumes normal program execution.
        /// </summary>
        /// <returns></returns>
        public async Task Resume()
        {
            await DoRequest(KiTag.REQ, KiRequest.Resume);
        }

        /// <summary>
        /// Executes the next line of code. If a function is called, the debugger
        /// will break at the first statement in that function.
        /// </summary>
        /// <returns></returns>
        public async Task StepInto()
        {
            await DoRequest(KiTag.REQ, KiRequest.StepIn);
        }

        /// <summary>
        /// Resumes normal execution until the current function returns.
        /// </summary>
        /// <returns></returns>
        public async Task StepOut()
        {
            await DoRequest(KiTag.REQ, KiRequest.StepOut);
        }

        /// <summary>
        /// Executes the next line of code.
        /// </summary>
        /// <returns></returns>
        public async Task StepOver()
        {
            await DoRequest(KiTag.REQ, KiRequest.StepOver);
        }

        private async Task<KiMessage> DoRequest(params dynamic[] values)
        {
            KiMessage message = new KiMessage(values);
            lock (replyLock)
                requests.Enqueue(message);
            message.Send(tcp.Client);

            return await Task.Run(() =>
            {
                while (true) {
                    lock (replyLock) {
                        if (replies.ContainsKey(message)) {
                            var reply = replies[message];
                            replies.Remove(message);
                            return reply;
                        }
                    }
                    Thread.Sleep(1);
                }
            });
        }

        private void ProcessMessages()
        {
            while (true) {
                KiMessage message = KiMessage.Receive(tcp.Client);
                if (message == null) {
                    // if DMessage.Receive() returns null, detach.
                    tcp.Close();
                    Detached?.Invoke(this, EventArgs.Empty);
                    return;
                }
                else if (message[0].Tag == KiTag.NFY) {
                    switch ((KiNotify)(int)message[1])
                    {
                        case KiNotify.Detaching:
                            tcp.Close();
                            Detached?.Invoke(this, EventArgs.Empty);
                            return;
                        case KiNotify.Log:
                            PrintType type = (PrintType)(int)message[2];
                            string debugText = (string)message[3];
                            string prefix = type == PrintType.Assert ? "ASSERT"
                                : type == PrintType.Debug ? "debug"
                                : type == PrintType.Error ? "ERROR"
                                : type == PrintType.Info ? "info"
                                : type == PrintType.Trace ? "trace"
                                : type == PrintType.Warn ? "warn"
                                : "log";
                            Print?.Invoke(this, new TraceEventArgs(string.Format("{0}: {1}", prefix, debugText)));
                            break;
                        case KiNotify.Pause:
                            FileName = (string)message[2];
                            LineNumber = (int)message[4];
                            Running = false;
                            Status?.Invoke(this, EventArgs.Empty);
                            break;
                        case KiNotify.Resume:
                            Running = true;
                            Status?.Invoke(this, EventArgs.Empty);
                            break;
                        case KiNotify.Throw:
                            Throw?.Invoke(this, new ThrowEventArgs(
                                (string)message[3], (string)message[4], (int)message[5],
                                (int)message[2] != 0));
                            break;
                    }
                }
                else if (message[0].Tag == KiTag.REP || message[0].Tag == KiTag.ERR) {
                    lock (replyLock) {
                        KiMessage request = requests.Dequeue();
                        replies.Add(request, message);
                    }
                }
            }
        }
    }
}
