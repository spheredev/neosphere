using System;
using System.IO;
using System.Windows.Forms;
using Microsoft.Win32;

using SphereStudio;
using SphereStudio.Base;

using Sphere.Gdk.Components;
using Sphere.Gdk.DockPanes;
using Sphere.Gdk.SettingsPages;
using Sphere.Gdk.Properties;

namespace Sphere.Gdk
{
    public class PluginMain : IPluginMain
    {
        public string Name { get; } = "miniSphere Support";
        public string Description { get; } = "Provides support for the miniSphere toolchain.";
        public string Version { get; } = "X.X.X";
        public string Author { get; } = "Fat Cerberus";

        internal PluginConf Conf { get; private set; }

        private ToolStripMenuItem m_sphereApiRefCommand;
        private ToolStripMenuItem m_cellApiRefCommand;
        private ToolStripMenuItem m_runtimeApiRefCommand;

        public void Initialize(ISettings conf)
        {
            Conf = new PluginConf(conf);

            PluginManager.Register(this, new miniSphereStarter(this), "miniSphere");
            PluginManager.Register(this, new CellCompiler(this), "Cell");
            PluginManager.Register(this, new SettingsPage(this), "Sphere GDK Setup");

            Panes.Initialize(this);

            m_sphereApiRefCommand = new ToolStripMenuItem("Sphere v2 Core API Reference", Resources.EvalIcon);
            m_sphereApiRefCommand.Click += sphereApiRefCommand_Click;
            m_runtimeApiRefCommand = new ToolStripMenuItem("Sphere Runtime API Reference", Resources.EvalIcon);
            m_runtimeApiRefCommand.Click += miniRTApiRefCommand_Click;
            m_cellApiRefCommand = new ToolStripMenuItem("Cellscript API Reference", Resources.EvalIcon);
            m_cellApiRefCommand.Click += cellApiRefCommand_Click;
            PluginManager.Core.AddMenuItem("Help", m_sphereApiRefCommand);
            PluginManager.Core.AddMenuItem("Help", m_runtimeApiRefCommand);
            PluginManager.Core.AddMenuItem("Help", m_cellApiRefCommand);

            PluginManager.Core.UnloadProject += on_UnloadProject;
        }

        public void ShutDown()
        {
            PluginManager.Core.UnloadProject -= on_UnloadProject;
            PluginManager.Core.RemoveMenuItem(m_sphereApiRefCommand);
            PluginManager.Core.RemoveMenuItem(m_runtimeApiRefCommand);
            PluginManager.Core.RemoveMenuItem(m_cellApiRefCommand);
            PluginManager.UnregisterAll(this);
        }

        private void sphereApiRefCommand_Click(object sender, EventArgs e)
        {
            string filePath = Path.Combine(Conf.GdkPath, "documentation", "sphere2-core-api.txt");
            PluginManager.Core.OpenFile(filePath);
        }

        private void miniRTApiRefCommand_Click(object sender, EventArgs e)
        {
            string filePath = Path.Combine(Conf.GdkPath, "documentation", "sphere2-hl-api.txt");
            PluginManager.Core.OpenFile(filePath);
        }

        private void cellApiRefCommand_Click(object sender, EventArgs e)
        {
            string filePath = Path.Combine(Conf.GdkPath, "documentation", "cellscript-api.txt");
            PluginManager.Core.OpenFile(filePath);
        }

        private void on_UnloadProject(object sender, EventArgs e)
        {
            Panes.Console.ClearErrors();
            Panes.Console.ClearConsole();
        }
    }

    class PluginConf
    {
        public PluginConf(ISettings conf)
        {
            Conf = conf;
        }

        public ISettings Conf { get; private set; }

        public string GdkPath
        {
            get
            {
                RegistryKey key = Registry.LocalMachine.OpenSubKey(
                    @"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{10C19C9F-1E29-45D8-A534-8FEF98C7C2FF}_is1");
                if (key != null)
                {
                    // miniSphere is installed, get path from registry
                    string defaultPath = (string)key.GetValue(@"InstallLocation") ?? "";
                    string path = Conf.GetString("gdkPath", defaultPath);
                    return !string.IsNullOrWhiteSpace(path) ? path : defaultPath;
                }
                else
                {
                    // no installation key, just read from conf
                    return Conf.GetString("gdkPath", "");
                }
            }
            set
            {
                Conf.SetValue("gdkPath", value);
            }
        }

        public bool AlwaysUseConsole
        {
            get { return Conf.GetBoolean("alwaysUseConsole", false); }
            set { Conf.SetValue("alwaysUseConsole", value); }
        }

        public bool MakeDebugPackages
        {
            get { return Conf.GetBoolean("makeDebugPackages", false); }
            set { Conf.SetValue("makeDebugPackages", value); }
        }

        public bool ShowTraceInfo
        {
            get { return Conf.GetBoolean("showTraceOutput", false); }
            set { Conf.SetValue("showTraceOutput", value); }
        }

        public bool TestInWindow
        {
            get { return Conf.GetBoolean("testInWindow", false); }
            set { Conf.SetValue("testInWindow", value); }
        }

        public int Verbosity
        {
            get { return Math.Min(Math.Max(Conf.GetInteger("verbosity", 0), 0), 4); }
            set { Conf.SetValue("verbosity", Math.Min(Math.Max(value, 0), 4)); }
        }
    }

    static class Panes
    {
        public static void Initialize(PluginMain main)
        {
            PluginManager.Register(main, Inspector = new InspectorPane(), "Inspector");
            PluginManager.Register(main, Console = new ConsolePane(main.Conf), "Debug Log");
        }

        public static ConsolePane Console { get; private set; }
        public static InspectorPane Inspector { get; private set; }
    }
}
