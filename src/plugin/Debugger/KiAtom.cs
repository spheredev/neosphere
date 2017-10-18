using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace Sphere.Gdk.Debugger
{
    enum KiTag
    {
        EOM = 0x00,
        REQ = 0x01,
        REP = 0x02,
        ERR = 0x03,
        NFY = 0x04,
        Integer = 0x10,
        String = 0x11,
        Buffer = 0x13,
        Undefined = 0x16,
        Null = 0x17,
        True = 0x18,
        False = 0x19,
        Number = 0x1A,
        Handle = 0x1F,
    }

    struct Handle
    {
        public uint Value;
    }

    class KiAtom
    {
        private KiTag m_tag;
        private dynamic m_value;

        public KiAtom(KiTag tag)
        {
            m_tag = tag;
        }

        public KiAtom(bool value)
        {
            m_tag = value ? KiTag.True : KiTag.False;
        }

        public KiAtom(byte[] buffer)
        {
            m_tag = KiTag.Buffer;
            m_value = buffer;
        }

        public KiAtom(double value)
        {
            m_tag = KiTag.Number;
            m_value = value;
        }

        public KiAtom(int value)
        {
            m_tag = KiTag.Integer;
            m_value = value;
        }

        public KiAtom(string value)
        {
            m_tag = KiTag.String;
            m_value = value;
        }

        public KiAtom(Handle handle)
        {
            m_tag = KiTag.Handle;
            m_value = handle;
        }

        public static explicit operator double(KiAtom dvalue)
        {
            return dvalue.m_tag == KiTag.Number ? dvalue.m_value
                : dvalue.m_tag == KiTag.Integer ? (double)dvalue.m_value
                : 0.0;
        }

        public static explicit operator int(KiAtom dvalue)
        {
            return dvalue.m_tag == KiTag.Integer ? dvalue.m_value
                : dvalue.m_tag == KiTag.Number ? (int)dvalue.m_value
                : 0;
        }

        public static explicit operator string(KiAtom dvalue)
        {
            return dvalue.m_tag == KiTag.String ? dvalue.m_value : "(unknown value)";
        }

        public static explicit operator Handle(KiAtom dvalue)
        {
            return dvalue.m_tag == KiTag.Handle ? dvalue.m_value : null;
        }

        public KiTag Tag
        {
            get { return m_tag; }
        }

        public static KiAtom Receive(Socket socket)
        {
            byte initialByte;
            byte[] bytes;
            int length = -1;
            Encoding utf8 = new UTF8Encoding(false);

            if (!socket.ReceiveAll(bytes = new byte[1]))
                return null;
            initialByte = bytes[0];
            Handle handle = new Handle();
            switch ((KiTag)initialByte)
            {
                case KiTag.EOM:
                case KiTag.REQ:
                case KiTag.REP:
                case KiTag.ERR:
                case KiTag.NFY:
                case KiTag.Undefined:
                case KiTag.Null:
                case KiTag.True:
                case KiTag.False:
                    return new KiAtom((KiTag)initialByte);
                case KiTag.Buffer:
                    if (!socket.ReceiveAll(bytes = new byte[4]))
                        return null;
                    length = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
                    if (!socket.ReceiveAll(bytes = new byte[length]))
                        return null;
                    return new KiAtom(bytes);
                case KiTag.Handle:
                    if (!socket.ReceiveAll(bytes = new byte[4]))
                        return null;
                    handle.Value = (uint)((bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3]);
                    return new KiAtom(handle);
                case KiTag.Integer:
                    if (!socket.ReceiveAll(bytes = new byte[4]))
                        return null;
                    return new KiAtom((bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3]);
                case KiTag.Number:
                    if (!socket.ReceiveAll(bytes = new byte[8]))
                        return null;
                    if (BitConverter.IsLittleEndian)
                        Array.Reverse(bytes);
                    return new KiAtom(BitConverter.ToDouble(bytes, 0));
                case KiTag.String:
                    if (!socket.ReceiveAll(bytes = new byte[4]))
                        return null;
                    length = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
                    if (!socket.ReceiveAll(bytes = new byte[length]))
                        return null;
                    return new KiAtom(utf8.GetString(bytes));
                default:
                    return null;
            }
        }

        public bool Send(Socket socket)
        {
            try {
                socket.Send(new byte[] { (byte)m_tag });
                Handle handle;
                switch (m_tag)
                {
                    case KiTag.Handle:
                        handle = (Handle)m_value;
                        socket.Send(new byte[] {
                            (byte)(handle.Value >> 24 & 0xFF),
                            (byte)(handle.Value >> 16 & 0xFF),
                            (byte)(handle.Value >> 8 & 0xFF),
                            (byte)(handle.Value & 0xFF)
                        });
                        break;
                    case KiTag.Integer:
                        socket.Send(new byte[] {
                            (byte)(m_value >> 24 & 0xFF),
                            (byte)(m_value >> 16 & 0xFF),
                            (byte)(m_value >> 8 & 0xFF),
                            (byte)(m_value & 0xFF)
                        });
                        break;
                    case KiTag.Number:
                        byte[] floatBytes = BitConverter.GetBytes(m_value);
                        if (BitConverter.IsLittleEndian)
                            Array.Reverse(floatBytes);
                        socket.Send(floatBytes);
                        break;
                    case KiTag.String:
                        var utf8 = new UTF8Encoding(false);
                        byte[] stringBytes = utf8.GetBytes(m_value);
                        socket.Send(new byte[] {
                            (byte)(stringBytes.Length >> 24 & 0xFF),
                            (byte)(stringBytes.Length >> 16 & 0xFF),
                            (byte)(stringBytes.Length >> 8 & 0xFF),
                            (byte)(stringBytes.Length & 0xFF)
                        });
                        socket.Send(stringBytes);
                        break;
                }
                return true;
            }
            catch (SocketException)
            {
                return false;
            }
        }

        public override string ToString()
        {
            return m_tag == KiTag.Handle ? "{ ... }"
                : m_tag == KiTag.Undefined ? "undefined"
                : m_tag == KiTag.Null ? "null"
                : m_tag == KiTag.True ? "true" : m_tag == KiTag.False ? "false"
                : m_tag == KiTag.Integer ? m_value.ToString()
                : m_tag == KiTag.Number ? m_value.ToString()
                : m_tag == KiTag.String ? string.Format("\"{0}\"", m_value.Replace(@"""", @"\""").Replace("\r", @"\r").Replace("\n", @"\n"))
                : "*munch*";
        }
    }
}
