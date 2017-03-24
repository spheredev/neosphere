using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace miniSphere.Gdk.Debugger
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
    /// Allows control of a Duktape debug target over TCP.
    /// </summary>
    class Inferior : IDisposable
    {
        private TcpClient tcp = new TcpClient() { NoDelay = true };
        private Thread messenger = null;
        private object replyLock = new object();
        private Queue<DMessage> requests = new Queue<DMessage>();
        private Dictionary<DMessage, DMessage> replies = new Dictionary<DMessage, DMessage>();
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
        /// Gets the target's Duktape version ID string.
        /// </summary>
        public string Version { get; private set; }
        
        /// <summary>
        /// Gets the filename reported in the last status update.
        /// </summary>
        public string FileName { get; private set; }

        /// <summary>
        /// Gets the line number being executed as of the last status update.
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
            string[] handshake = line.Trim().Split(new[] { ' ' }, 4);
            protocolVersion = int.Parse(handshake[0]);
            if (protocolVersion < 1 || protocolVersion > 2)
                throw new NotSupportedException("Unsupported Duktape debugger protocol version!");

            Version = handshake[2];
            TargetID = handshake[3];

            // start the communication thread
            messenger = new Thread(ProcessMessages) { IsBackground = true };
            messenger.Start();
            await DoRequest(DValueTag.REQ, Request.AppRequest, AppRequest.SetWatermark, "ssj blue", 0, 255, 255);
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
            var reply = await DoRequest(DValueTag.REQ, Request.AddBreak, filename, lineNumber);
            return (int)reply[1];
        }

        /// <summary>
        /// Clears the breakpoint with the specified index.
        /// </summary>
        /// <param name="index">The index of the breakpoint to clear, as returned by AddBreak.</param>
        /// <returns></returns>
        public async Task DelBreak(int index)
        {
            await DoRequest(DValueTag.REQ, Request.DelBreak, index);
        }
        
        /// <summary>
        /// Requests that Duktape end the debug session.
        /// </summary>
        /// <returns></returns>
        public async Task Detach()
        {
            if (messenger == null)
                return;
            await DoRequest(DValueTag.REQ, Request.Detach);
            await Task.Run(() => messenger.Join());
            tcp.Client.Disconnect(true);
            Detached?.Invoke(this, EventArgs.Empty);
        }

        /// <summary>
        /// Evaluates a JS expression and returns the JX-encoded result.
        /// </summary>
        /// <param name="expression">The expression or statement to evaluate.</param>
        /// <param name="stackOffset">The point in the stack to do the eval. -1 is active call, -2 the caller, etc..</param>
        /// <returns>The value produced by the expression.</returns>
        public async Task<DValue> Eval(string expression, int stackOffset = -1)
        {
            var reply = protocolVersion == 2
                ? await DoRequest(DValueTag.REQ, Request.Eval, stackOffset, expression)
                : await DoRequest(DValueTag.REQ, Request.Eval, expression, stackOffset);
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
            var reply = await DoRequest(DValueTag.REQ, Request.GetCallStack);
            var stack = new List<StackFrameInfo>();
            int count = (reply.Length - 1) / 4;
            for (int i = 0; i < count; ++i) {
                string filename = (string)reply[1 + i * 4];
                string functionName = (string)reply[2 + i * 4];
                int lineNumber = (int)reply[3 + i * 4];
                int pc = (int)reply[4 + i * 4];
                if (lineNumber == 0)
                    filename = "[system call]";
                stack.Add(new StackFrameInfo(functionName, filename, lineNumber));
            }
            return stack.ToArray();
        }

        /// <summary>
        /// Gets a list of local values and their values. Note that objects
        /// are not evaluated and are listed simply as "{ obj: 'ClassName' }".
        /// </summary>
        /// <param name="stackOffset">The call stack offset to get locals for, -1 being the current activation.</param>
        /// <returns></returns>
        public async Task<IReadOnlyDictionary<string, DValue>> GetLocals(int stackOffset = -1)
        {
            var reply = await DoRequest(DValueTag.REQ, Request.GetLocals, stackOffset);
            var vars = new Dictionary<string, DValue>();
            int count = (reply.Length - 1) / 2;
            for (int i = 0; i < count; ++i) {
                string name = (string)reply[1 + i * 2];
                DValue value = reply[2 + i * 2];
                vars.Add(name, value);
            }
            return vars;
        }

        public async Task<Dictionary<string, PropDesc>> GetObjPropDescRange(HeapPtr ptr, int start, int end)
        {
            var reply = await DoRequest(DValueTag.REQ, Request.GetObjPropDescRange, ptr, start, end);
            var props = new Dictionary<string, PropDesc>();
            int count = (reply.Length - 1) / 2;
            int i = 1;
            while (i < reply.Length) {
                PropFlags flags = (PropFlags)(int)reply[i++];
                string name = reply[i++].ToString();
                if (flags.HasFlag(PropFlags.Accessor)) {
                    DValue getter = reply[i++];
                    DValue setter = reply[i++];
                    PropDesc propValue = new PropDesc(getter, setter, flags);
                    if (!flags.HasFlag(PropFlags.Internal))
                        props.Add(name, propValue);
                }
                else {
                    DValue value = reply[i++];
                    PropDesc propValue = new PropDesc(value, flags);
                    if (!flags.HasFlag(PropFlags.Internal) && value.Tag != DValueTag.Unused)
                        props.Add(name, propValue);
                }
            }
            return props;
        }

        public async Task<string> GetSource(string fileName)
        {
            var reply = await DoRequest(DValueTag.REQ, Request.AppRequest, AppRequest.GetSource, fileName);
            return reply[0].Tag != DValueTag.ERR ? (string)reply[1] : null;
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
            var reply = await DoRequest(DValueTag.REQ, Request.ListBreak);
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
            await DoRequest(DValueTag.REQ, Request.Pause);
        }

        /// <summary>
        /// Resumes normal program execution.
        /// </summary>
        /// <returns></returns>
        public async Task Resume()
        {
            await DoRequest(DValueTag.REQ, Request.Resume);
        }

        /// <summary>
        /// Executes the next line of code. If a function is called, the debugger
        /// will break at the first statement in that function.
        /// </summary>
        /// <returns></returns>
        public async Task StepInto()
        {
            await DoRequest(DValueTag.REQ, Request.StepInto);
        }

        /// <summary>
        /// Resumes normal execution until the current function returns.
        /// </summary>
        /// <returns></returns>
        public async Task StepOut()
        {
            await DoRequest(DValueTag.REQ, Request.StepOut);
        }

        /// <summary>
        /// Executes the next line of code.
        /// </summary>
        /// <returns></returns>
        public async Task StepOver()
        {
            await DoRequest(DValueTag.REQ, Request.StepOver);
        }

        private async Task<DMessage> DoRequest(params dynamic[] values)
        {
            DMessage message = new DMessage(values);
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
                DMessage message = DMessage.Receive(tcp.Client);
                if (message == null) {
                    // if DMessage.Receive() returns null, detach.
                    tcp.Close();
                    Detached?.Invoke(this, EventArgs.Empty);
                    return;
                }
                else if (message[0].Tag == DValueTag.NFY) {
                    switch ((Notify)(int)message[1])
                    {
                        case Notify.Status:
                            FileName = (string)message[3];
                            LineNumber = (int)message[5];
                            Running = (int)message[2] == 0;
                            Status?.Invoke(this, EventArgs.Empty);
                            break;
                        case Notify.Throw:
                            Throw?.Invoke(this, new ThrowEventArgs(
                                (string)message[3], (string)message[4], (int)message[5],
                                (int)message[2] != 0));
                            break;
                        case Notify.Detaching:
                            tcp.Close();
                            Detached?.Invoke(this, EventArgs.Empty);
                            return;
                        case Notify.AppNotify:
                            switch ((AppNotify)(int)message[2])
                            {
                                case AppNotify.DebugPrint:
                                    PrintType type = (PrintType)(int)message[3];
                                    string debugText = (string)message[4];
                                    string prefix = type == PrintType.Assert ? "ASSERT"
                                        : type == PrintType.Debug ? "debug"
                                        : type == PrintType.Error ? "ERROR"
                                        : type == PrintType.Info ? "info"
                                        : type == PrintType.Trace ? "trace"
                                        : type == PrintType.Warn ? "warn"
                                        : "log";
                                    Print?.Invoke(this, new TraceEventArgs(string.Format("{0}: {1}", prefix, debugText)));
                                    break;
                            }
                            break;
                    }
                }
                else if (message[0].Tag == DValueTag.REP || message[0].Tag == DValueTag.ERR) {
                    lock (replyLock) {
                        DMessage request = requests.Dequeue();
                        replies.Add(request, message);
                    }
                }
            }
        }
    }
}
