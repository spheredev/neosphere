using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace Sphere.Gdk.Debugger
{
    enum KiError
    {
        None,
        NotFound,
        TooMany,
        Unsupported,
    }

    enum KiNotify
    {
        None,
        Detaching,
        Log,
        Pause,
        Resume,
        Throw,
    }

    enum KiRequest
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

    class KiMessage
    {
        private KiAtom[] m_words;

        public KiMessage(params dynamic[] values)
        {
            List<KiAtom> fields = new List<KiAtom>();
            KiAtom fieldValue;

            foreach (dynamic value in values) {
                if (value is KiAtom)
                    fieldValue = value;
                else if (value is KiRequest)
                    fieldValue = new KiAtom((int)value);
                else
                    fieldValue = new KiAtom(value);
                fields.Add(fieldValue);
            }
            m_words = fields.ToArray();
        }

        public KiAtom this[int index] => m_words[index];

        public int Length => m_words.Length;

        public static KiMessage Receive(Socket socket)
        {
            List<KiAtom> fields = new List<KiAtom>();
            KiAtom value;

            do {
                if ((value = KiAtom.Receive(socket)) == null)
                    return null;
                if (value.Tag != KiTag.EOM)
                    fields.Add(value);
            } while (value.Tag != KiTag.EOM);
            return new KiMessage(fields.ToArray());
        }

        public bool Send(Socket socket)
        {
            foreach (var field in m_words) {
                if (!field.Send(socket))
                    return false;
            }
            if (!(new KiAtom(KiTag.EOM).Send(socket)))
                return false;
            return true;
        }
    }
}
