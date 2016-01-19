using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace minisphere.Gdk
{
    static class SocketExtensions
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
    }
}
