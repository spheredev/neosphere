using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace minisphere.Gdk.Debugger
{
    struct JSProperty
    {
        public JSProperty(dynamic value, JSPropFlags flags)
        {
            Value = value;
            Getter = Setter = null;
            Flags = flags;
        }

        public JSProperty(dynamic getter, dynamic setter, JSPropFlags flags)
        {
            Value = null;
            Getter = getter;
            Setter = setter;
            Flags = flags;
        }

        public dynamic Value;
        public dynamic Getter;
        public dynamic Setter;
        public JSPropFlags Flags;
    }

    [Flags]
    enum JSPropFlags
    {
        None = 0x0000,
        Writable = 0x0001,
        Enumerable = 0x0002,
        Configurable = 0x0004,
        Accessor = 0x0008,
        Virtual = 0x0010,
        Internal = 0x0100,
    }
}
