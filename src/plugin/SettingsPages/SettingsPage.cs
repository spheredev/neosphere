using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using Sphere.Plugins.Interfaces;
using Sphere.Plugins;

namespace miniSphere.Gdk.SettingsPages
{
    partial class SettingsPage : UserControl, ISettingsPage
    {
        private PluginMain _main;

        public SettingsPage(PluginMain main)
        {
            InitializeComponent();
            _main = main;
        }

        public Control Control { get { return this; } }

        public bool Apply()
        {
            _main.Conf.GdkPath = GdkPathTextBox.Text;
            _main.Conf.MakeDebugPackages = MakeDebugPackageCheckBox.Checked;
            _main.Conf.AlwaysUseConsole = TestWithConsoleCheckBox.Checked;
            _main.Conf.ShowTraceInfo = ShowTraceCheckBox.Checked;
            _main.Conf.TestInWindow = TestInWindowCheckBox.Checked;
            _main.Conf.Verbosity = VerbosityComboBox.SelectedIndex;
            return true;
        }

        protected override void OnLoad(EventArgs e)
        {
            GdkPathTextBox.Text = _main.Conf.GdkPath;
            MakeDebugPackageCheckBox.Checked = _main.Conf.MakeDebugPackages;
            ShowTraceCheckBox.Checked = _main.Conf.ShowTraceInfo;
            TestWithConsoleCheckBox.Checked = _main.Conf.AlwaysUseConsole;
            TestInWindowCheckBox.Checked = _main.Conf.TestInWindow;
            VerbosityComboBox.SelectedIndex = _main.Conf.Verbosity;
            base.OnLoad(e);
        }

        private void BrowseButton_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog fb = new FolderBrowserDialog();
            fb.Description = "Select the folder where the miniSphere GDK is installed.";
            fb.ShowNewFolderButton = false;
            if (fb.ShowDialog(this) == DialogResult.OK)
            {
                GdkPathTextBox.Text = fb.SelectedPath;
            }
        }
    }
}
