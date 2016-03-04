using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Sphere.Plugins;
using Sphere.Plugins.Interfaces;

namespace minisphere.Gdk.Plugins
{
    class minisphereStarter : IDebugStarter
    {
        private PluginMain _main;

        public minisphereStarter(PluginMain main)
        {
            _main = main;
        }

        public bool CanConfigure { get { return false; } }

        public void Start(string gamePath, bool isPackage)
        {
            string gdkPath = _main.Conf.GdkPath;
            bool wantConsole = _main.Conf.AlwaysUseConsole;
            bool wantWindow = _main.Conf.TestInWindow || wantConsole;

            string enginePath = Path.Combine(gdkPath, wantConsole ? "spherun.exe" : "minisphere.exe");
            string options = string.Format(@"{1} ""{0}""", gamePath,
                wantWindow ? "--window" : "--fullscreen");
            Process.Start(enginePath, options);
        }

        public void Configure()
        {
            throw new NotSupportedException("minisphere doesn't support engine configuration.");
        }

        public IDebugger Debug(string gamePath, bool isPackage, IProject project)
        {
            string gdkPath = _main.Conf.GdkPath;

            string enginePath = Path.Combine(gdkPath, "spherun.exe");
            string options = string.Format(@"--debug ""{0}""", gamePath);
            Process engine = Process.Start(enginePath, options);
            return new Ssj2Debugger(_main, gamePath, enginePath, engine, project);
        }
    }
}
