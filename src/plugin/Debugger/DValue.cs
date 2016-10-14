using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace minisphere.Gdk.Debugger
{
    enum DValueTag
    {
        EOM = 0x00,
        REQ = 0x01,
        REP = 0x02,
        ERR = 0x03,
        NFY = 0x04,
        Integer = 0x10,
        String = 0x11,
        SmallString = 0x12,
        Buffer = 0x13,
        SmallBuffer = 0x14,
        Unused = 0x15,
        Undefined = 0x16,
        Null = 0x17,
        True = 0x18,
        False = 0x19,
        Float = 0x1A,
        Object = 0x1B,
        Pointer = 0x1C,
        LightFunc = 0x1D,
        HeapPtr = 0x1E,
    }

    enum HeapClass
    {
        Unknown,
        Arguments,
        Array,
        Boolean,
        Date,
        Error,
        Function,
        JSON,
        Math,
        Number,
        Object,
        RegExp,
        String,
        Global,
        ObjEnv,
        DecEnv,
        Pointer,
        Thread,
        ArrayBuffer,
        DataView,
        Int8Array,
        Uint8Array,
        Uint8ClampedArray,
        Int16Array,
        Uint16Array,
        Int32Array,
        Uint32Array,
        Float32Array,
        Float64Array,
    }

    struct HeapPtr
    {
        public HeapClass Class;
        public byte[] Data;
        public byte Size;
    }

    class DValue
    {
        private DValueTag _tag;
        private dynamic _value;

        public DValue(DValueTag value)
        {
            _tag = value;
        }

        public DValue(byte[] buffer)
        {
            _tag = DValueTag.Buffer;
            _value = buffer;
        }

        public DValue(double value)
        {
            _tag = DValueTag.Float;
            _value = value;
        }

        public DValue(int value)
        {
            _tag = DValueTag.Integer;
            _value = value;
        }

        public DValue(string value)
        {
            _tag = DValueTag.String;
            _value = value;
        }

        public DValue(HeapPtr ptr)
        {
            _tag = DValueTag.HeapPtr;
            _value = ptr;
        }

        public static explicit operator double(DValue dvalue)
        {
            return dvalue._tag == DValueTag.Float ? dvalue._value
                : dvalue._tag == DValueTag.Integer ? (double)dvalue._value
                : 0.0;
        }

        public static explicit operator int(DValue dvalue)
        {
            return dvalue._tag == DValueTag.Integer ? dvalue._value
                : dvalue._tag == DValueTag.Float ? (int)dvalue._value
                : 0;
        }

        public static explicit operator string(DValue dvalue)
        {
            return dvalue._tag == DValueTag.String ? dvalue._value : "(unknown value)";
        }

        public static explicit operator HeapPtr(DValue dvalue)
        {
            return dvalue._tag == DValueTag.HeapPtr ? dvalue._value : null;
        }

        public DValueTag Tag
        {
            get { return _tag; }
        }

        public static DValue Receive(Socket socket)
        {
            byte initialByte;
            byte[] bytes;
            int length = -1;
            Encoding utf8 = new UTF8Encoding(false);

            if (!socket.ReceiveAll(bytes = new byte[1]))
                return null;
            initialByte = bytes[0];
            if (initialByte >= 0x60 && initialByte < 0x80)
            {
                length = initialByte - 0x60;
                if (!socket.ReceiveAll(bytes = new byte[length]))
                    return null;
                return new DValue(utf8.GetString(bytes));
            }
            else if (initialByte >= 0x80 && initialByte < 0xC0)
            {
                return new DValue(initialByte - 0x80);
            }
            else if (initialByte >= 0xC0)
            {
                Array.Resize(ref bytes, 2);
                if (socket.Receive(bytes, 1, 1, SocketFlags.None) == 0)
                    return null;
                return new DValue(((initialByte - 0xC0) << 8) + bytes[1]);
            }
            else
            {
                HeapPtr heapPtr = new HeapPtr();
                switch ((DValueTag)initialByte)
                {
                    case DValueTag.EOM:
                    case DValueTag.REQ:
                    case DValueTag.REP:
                    case DValueTag.ERR:
                    case DValueTag.NFY:
                    case DValueTag.Unused:
                    case DValueTag.Undefined:
                    case DValueTag.Null:
                    case DValueTag.True:
                    case DValueTag.False:
                        return new DValue((DValueTag)initialByte);
                    case DValueTag.Integer: // 32-bit integer
                        if (!socket.ReceiveAll(bytes = new byte[4]))
                            return null;
                        return new DValue((bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3]);
                    case DValueTag.String:
                        if (!socket.ReceiveAll(bytes = new byte[4]))
                            return null;
                        length = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
                        if (!socket.ReceiveAll(bytes = new byte[length]))
                            return null;
                        return new DValue(utf8.GetString(bytes));
                    case DValueTag.SmallString:
                        if (!socket.ReceiveAll(bytes = new byte[2]))
                            return null;
                        length = (bytes[0] << 8) + bytes[1];
                        if (!socket.ReceiveAll(bytes = new byte[length]))
                            return null;
                        return new DValue(utf8.GetString(bytes));
                    case DValueTag.Buffer:
                        if (!socket.ReceiveAll(bytes = new byte[4]))
                            return null;
                        length = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
                        if (!socket.ReceiveAll(bytes = new byte[length]))
                            return null;
                        return new DValue(bytes);
                    case DValueTag.SmallBuffer:
                        if (!socket.ReceiveAll(bytes = new byte[2]))
                            return null;
                        length = (bytes[0] << 8) + bytes[1];
                        if (!socket.ReceiveAll(bytes = new byte[length]))
                            return null;
                        return new DValue(bytes);
                    case DValueTag.Float:
                        if (!socket.ReceiveAll(bytes = new byte[8]))
                            return null;
                        if (BitConverter.IsLittleEndian)
                            Array.Reverse(bytes);
                        return new DValue(BitConverter.ToDouble(bytes, 0));
                    case DValueTag.Object:
                        socket.ReceiveAll(bytes = new byte[1]);
                        heapPtr.Class = (HeapClass)bytes[0];
                        socket.ReceiveAll(bytes = new byte[1]);
                        heapPtr.Size = bytes[0];
                        socket.ReceiveAll(heapPtr.Data = new byte[heapPtr.Size]);
                        return new DValue(heapPtr);
                    case DValueTag.Pointer:
                        socket.ReceiveAll(bytes = new byte[1]);
                        socket.ReceiveAll(new byte[bytes[0]]);
                        return new DValue(DValueTag.Pointer);
                    case DValueTag.LightFunc:
                        socket.ReceiveAll(bytes = new byte[3]);
                        socket.ReceiveAll(new byte[bytes[2]]);
                        return new DValue(DValueTag.LightFunc);
                    case DValueTag.HeapPtr:
                        heapPtr.Class = HeapClass.Unknown;
                        socket.ReceiveAll(bytes = new byte[1]);
                        heapPtr.Size = bytes[0];
                        socket.ReceiveAll(heapPtr.Data = new byte[heapPtr.Size]);
                        return new DValue(heapPtr);
                    default:
                        return null;
                }
            }
        }

        public bool Send(Socket socket)
        {
            try {
                socket.Send(new byte[] { (byte)_tag });
                switch (_tag)
                {
                    case DValueTag.Float:
                        byte[] floatBytes = BitConverter.GetBytes(_value);
                        if (BitConverter.IsLittleEndian)
                            Array.Reverse(floatBytes);
                        socket.Send(floatBytes);
                        break;
                    case DValueTag.Integer:
                        socket.Send(new byte[] {
                            (byte)(_value >> 24 & 0xFF),
                            (byte)(_value >> 16 & 0xFF),
                            (byte)(_value >> 8 & 0xFF),
                            (byte)(_value & 0xFF)
                        });
                        break;
                    case DValueTag.String:
                        var utf8 = new UTF8Encoding(false);
                        byte[] stringBytes = utf8.GetBytes(_value);
                        socket.Send(new byte[] {
                            (byte)(stringBytes.Length >> 24 & 0xFF),
                            (byte)(stringBytes.Length >> 16 & 0xFF),
                            (byte)(stringBytes.Length >> 8 & 0xFF),
                            (byte)(stringBytes.Length & 0xFF)
                        });
                        socket.Send(stringBytes);
                        break;
                    case DValueTag.HeapPtr:
                        socket.Send(new byte[] { _value.Size });
                        socket.Send(_value.Data);
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
            return
                _tag == DValueTag.HeapPtr && _value.Class == HeapClass.Array ? "[ ... ]"
                : _tag == DValueTag.HeapPtr ? string.Format(@"{{ obj: '{0}' }}", _value.Class.ToString())
                : _tag == DValueTag.Undefined ? "undefined"
                : _tag == DValueTag.Null ? "null"
                : _tag == DValueTag.True ? "true" : _tag == DValueTag.False ? "false"
                : _tag == DValueTag.Integer ? _value.ToString()
                : _tag == DValueTag.Float ? _value.ToString()
                : _tag == DValueTag.String ? string.Format("\"{0}\"", _value.Replace(@"""", @"\""").Replace("\r", @"\r").Replace("\n", @"\n"))
                : "*munch*";
        }
    }
}
