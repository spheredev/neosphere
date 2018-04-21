using System;
using System.Diagnostics;
using System.IO;

using SphereStudio.Base;

namespace Sphere.Gdk.Components
{
    class miniSphereStarter : IDebugStarter
    {
        private PluginMain m_main;

        public miniSphereStarter(PluginMain main)
        {
            m_main = main;
        }

        public bool CanConfigure { get { return false; } }

        public void Start(string gamePath, bool isPackage)
        {
            string gdkPath = m_main.Conf.GdkPath;
            bool wantConsole = m_main.Conf.AlwaysUseConsole;
            bool wantWindow = m_main.Conf.TestInWindow || wantConsole;

            string enginePath = Path.Combine(gdkPath, wantConsole ? "spherun.exe" : "minisphere.exe");
            string options = string.Format(@"{0} --verbose {1} {2} {3} ""{4}""",
                wantWindow ? "--windowed" : "",
                m_main.Conf.Verbosity,
                wantConsole && m_main.Conf.UseRetroMode ? "--retro" : "",
                wantConsole ? "--profile" : "",
                gamePath);
            Process.Start(enginePath, options);
        }

        public void Configure()
        {
            throw new NotSupportedException("miniSphere doesn't support engine configuration.");
        }

        public IDebugger Debug(string gamePath, bool isPackage, IProject project)
        {
            string gdkPath = m_main.Conf.GdkPath;

            PluginManager.Core.Docking.Activate(Panes.Console);
            Panes.Console.ClearConsole();
            PluginManager.Core.Docking.Show(Panes.Inspector);
            string enginePath = Path.Combine(gdkPath, "spherun.exe");
            string options = string.Format(@"--verbose {0} {1} --debug ""{2}""",
                m_main.Conf.Verbosity,
                m_main.Conf.UseRetroMode ? "--retro" : "",
                gamePath);
            Process engine = Process.Start(enginePath, options);
            return new SsjDebugger(m_main, gamePath, enginePath, engine, project);
        }
    }
}
