using System.Diagnostics;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

namespace miniSphere.Gdk
{
    static class Extensions
    {
        public static bool ReceiveAll(this Socket socket, byte[] buffer, SocketFlags socketFlags = SocketFlags.None)
        {
            try
            {
                int offset = 0;
                while (offset < buffer.Length)
                {
                    int size = socket.Receive(buffer, offset, buffer.Length - offset, socketFlags);
                    offset += size;
                    if (size == 0) return false;
                }
                return true;
            }
            catch (SocketException)
            {
                return false;
            }
        }

        public static Task WaitForExitAsync(this Process process,
                                            CancellationToken cancelToken = default(CancellationToken))
        {
            var tcs = new TaskCompletionSource<object>();
            process.EnableRaisingEvents = true;
            process.Exited += (sender, e) => tcs.TrySetResult(null);
            if (cancelToken != default(CancellationToken))
            {
                cancelToken.Register(() => tcs.TrySetCanceled());
            }
            return tcs.Task;
        }

        private static void Process_Exited(object sender, System.EventArgs e)
        {
            throw new System.NotImplementedException();
        }
    }
}
