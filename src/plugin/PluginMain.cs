using System;
using System.IO;
using System.Windows.Forms;
using Microsoft.Win32;

using Sphere.Plugins;
using Sphere.Plugins.Interfaces;

using minisphere.Gdk.DockPanes;
using minisphere.Gdk.Plugins;
using minisphere.Gdk.SettingsPages;
using minisphere.Gdk.Properties;

namespace minisphere.Gdk
{
    public class PluginMain : IPluginMain
    {
        public string Name { get; } = "minisphere GDK";
        public string Author { get; } = "Fat Cerberus";
        public string Description { get; } = "Provides support for the minisphere GDK toolchain.";
        public string Version { get; } = "4.0.1";

        internal PluginConf Conf { get; private set; }
        internal int Sessions { get; set; }

        private ToolStripMenuItem _sphereApiRefCommand;
        private ToolStripMenuItem _cellApiRefCommand;
        private ToolStripMenuItem _miniRTApiRefCommand;

        public void Initialize(ISettings conf)
        {
            Conf = new PluginConf(conf);
            Sessions = 0;

            PluginManager.Register(this, new minisphereStarter(this), "minisphere");
            PluginManager.Register(this, new CellCompiler(this), "Cell");
            PluginManager.Register(this, new SettingsPage(this), "minisphere Setup");

            Panes.Initialize(this);

            _sphereApiRefCommand = new ToolStripMenuItem("Sphere v2 API Reference", Resources.EvalIcon);
            _sphereApiRefCommand.Click += sphereApiRefCommand_Click;
            _miniRTApiRefCommand = new ToolStripMenuItem("miniRT API Reference", Resources.EvalIcon);
            _miniRTApiRefCommand.Click += miniRTApiRefCommand_Click;
            _cellApiRefCommand = new ToolStripMenuItem("Cellscript API Reference", Resources.EvalIcon);
            _cellApiRefCommand.Click += cellApiRefCommand_Click;
            PluginManager.Core.AddMenuItem("Help", _sphereApiRefCommand);
            PluginManager.Core.AddMenuItem("Help", _miniRTApiRefCommand);
            PluginManager.Core.AddMenuItem("Help", _cellApiRefCommand);

            PluginManager.Core.UnloadProject += on_UnloadProject;
        }

        public void ShutDown()
        {
            PluginManager.Core.UnloadProject -= on_UnloadProject;
            PluginManager.Core.RemoveMenuItem(_sphereApiRefCommand);
            PluginManager.Core.RemoveMenuItem(_miniRTApiRefCommand);
            PluginManager.Core.RemoveMenuItem(_cellApiRefCommand);
            PluginManager.UnregisterAll(this);
        }

        private void sphereApiRefCommand_Click(object sender, EventArgs e)
        {
            string filePath = Path.Combine(Conf.GdkPath, "documentation", "sphere2-api.txt");
            PluginManager.Core.OpenFile(filePath);
        }

        private void miniRTApiRefCommand_Click(object sender, EventArgs e)
        {
            string filePath = Path.Combine(Conf.GdkPath, "documentation", "miniRT-api.txt");
            PluginManager.Core.OpenFile(filePath);
        }

        private void cellApiRefCommand_Click(object sender, EventArgs e)
        {
            string filePath = Path.Combine(Conf.GdkPath, "documentation", "cellscript-api.txt");
            PluginManager.Core.OpenFile(filePath);
        }

        private void on_UnloadProject(object sender, EventArgs e)
        {
            Panes.Errors.Clear();
            Panes.Console.Clear();
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
                    // minisphere GDK is installed, get path from registry
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
            get { return Math.Min(Math.Max(Conf.GetInteger("verbosity", 2), 0), 4); }
            set { Conf.SetValue("verbosity", Math.Min(Math.Max(value, 0), 4)); }
        }
    }

    static class Panes
    {
        public static void Initialize(PluginMain main)
        {
            PluginManager.Register(main, Inspector = new InspectorPane(), "Debugger");
            PluginManager.Register(main, Console = new ConsolePane(main.Conf), "Console");
            PluginManager.Register(main, Errors = new ErrorPane(), "Exceptions");
        }

        public static ConsolePane Console { get; private set; }
        public static ErrorPane Errors { get; private set; }
        public static InspectorPane Inspector { get; private set; }
    }
}
