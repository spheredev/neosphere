using System.Collections.Generic;
using System.Net.Sockets;

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
        Detach,
        Log,
        Pause,
        Resume,
        Throw,
    }

    enum KiRequest
    {
        None,
        AddBreakpoint,
        AssetData,
        DeleteBreakpoint,
        Detach,
        Eval,
        GameInfo,
        InspectBreakpoints,
        InspectLocals,
        InspectObject,
        InspectStack,
        Pause,
        Resume,
        StepIn,
        StepOut,
        StepOver,
        Watermark,
    }

    enum LogOp
    {
        Normal,
        Trace,
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
