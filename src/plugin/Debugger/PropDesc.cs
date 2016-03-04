using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace minisphere.Gdk.Debugger
{
    struct PropDesc
    {
        public PropDesc(DValue value, PropFlags flags)
        {
            Value = value;
            Getter = Setter = null;
            Flags = flags;
        }

        public PropDesc(DValue getter, DValue setter, PropFlags flags)
        {
            Value = null;
            Getter = getter;
            Setter = setter;
            Flags = flags;
        }

        public DValue Value;
        public DValue Getter;
        public DValue Setter;
        public PropFlags Flags;
    }

    [Flags]
    enum PropFlags
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
