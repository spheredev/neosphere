using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace minisphere.Gdk.Debugger
{
    enum Request
    {
        BasicInfo = 0x10,
        TriggerStatus = 0x11,
        Pause = 0x12,
        Resume = 0x13,
        StepInto = 0x14,
        StepOver = 0x15,
        StepOut = 0x16,
        ListBreak = 0x17,
        AddBreak = 0x18,
        DelBreak = 0x19,
        GetVar = 0x1A,
        PutVar = 0x1B,
        GetCallStack = 0x1C,
        GetLocals = 0x1D,
        Eval = 0x1E,
        Detach = 0x1F,
        DumpHeap = 0x20,
        GetByteCode = 0x21,
        AppRequest = 0x22,
        GetHeapObjInfo = 0x23,
        GetObjPropDesc = 0x24,
        GetObjPropDescRange = 0x25,
    }

    enum Notify
    {
        Status = 0x01,
        Print = 0x02,
        Alert = 0x03,
        Log = 0x04,
        Throw = 0x05,
        Detaching = 0x06,
        AppNotify = 0x07,
    }

    enum AppRequest
    {
        GetGameInfo = 0x01,
        GetSource = 0x02,
    }

    enum AppNotify
    {
        DebugPrint = 0x01,
    }

    class DMessage
    {
        private DValue[] _fields;

        public DMessage(params dynamic[] values)
        {
            List<DValue> fields = new List<DValue>();
            DValue fieldValue;

            foreach (dynamic value in values) {
                if (value is DValue)
                    fieldValue = value;
                else if (value is Request || value is AppRequest)
                    fieldValue = new DValue((int)value);
                else
                    fieldValue = new DValue(value);
                fields.Add(fieldValue);
            }
            _fields = fields.ToArray();
        }

        public DValue this[int index] => _fields[index];

        public int Length => _fields.Length;

        public static DMessage Receive(Socket socket)
        {
            List<DValue> fields = new List<DValue>();
            DValue value;

            do {
                if ((value = DValue.Receive(socket)) == null)
                    return null;
                if (value.Tag != DValueTag.EOM)
                    fields.Add(value);
            } while (value.Tag != DValueTag.EOM);
            return new DMessage(fields.ToArray());
        }

        public bool Send(Socket socket)
        {
            foreach (var field in _fields) {
                if (!field.Send(socket))
                    return false;
            }
            if (!(new DValue(DValueTag.EOM).Send(socket)))
                return false;
            return true;
        }
    }
}
