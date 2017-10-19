using System;
using System.Runtime.InteropServices;

namespace Sphere.Gdk
{
    static class NativeMethods
    {
        [DllImport("user32.dll")] [return: MarshalAs(UnmanagedType.Bool)]
        internal static extern bool SetForegroundWindow(IntPtr hWnd);
    }
}
