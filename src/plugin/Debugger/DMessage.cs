using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace Sphere.Gdk.Debugger
{
    enum Request
    {
        None,
        AddBreakpoint,
        DelBreakpoint,
        Detach,
        Eval,
        GetBreakpoints,
        GetGameInfo,
        GetSource,
        InspectLocals,
        InspectObject,
        InspectStack,
        Pause,
        Resume,
        SetWatermark,
        StepIn,
        StepOut,
        StepOver,
    }

    enum Notify
    {
        None,
        Detaching,
        Log,
        Pause,
        Resume,
        Throw,
    }

    enum PrintType
    {
        Normal = 0x00,
        Assert = 0x01,
        Debug = 0x02,
        Error = 0x03,
        Info = 0x04,
        Trace = 0x05,
        Warn = 0x06,
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
                else if (value is Request)
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
