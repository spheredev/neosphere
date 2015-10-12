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

namespace minisphere.Gdk.SettingsPages
{
    partial class SettingsPage : UserControl, ISettingsPage
    {
        private PluginConf _settings;

        public SettingsPage(PluginConf settings)
        {
            InitializeComponent();
            _settings = settings;
        }

        public Control Control { get { return this; } }

        public bool Apply()
        {
            _settings.GdkPath = GdkPathTextBox.Text;
            _settings.MakeDebugPackages = MakeDebugPackageCheckBox.Checked;
            _settings.KeepDebugOutput = KeepOutputCheckBox.Checked;
            _settings.AlwaysUseConsole = TestWithConsoleCheckBox.Checked;
            return true;
        }

        protected override void OnLoad(EventArgs e)
        {
            GdkPathTextBox.Text = _settings.GdkPath;
            MakeDebugPackageCheckBox.Checked = _settings.MakeDebugPackages;
            KeepOutputCheckBox.Checked = _settings.KeepDebugOutput;
            TestWithConsoleCheckBox.Checked = _settings.AlwaysUseConsole;

            base.OnLoad(e);
        }

        private void BrowseButton_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog fb = new FolderBrowserDialog();
            fb.Description = "Select the folder where the minisphere GDK is installed. (minisphere 2.0+ required)";
            fb.ShowNewFolderButton = false;
            if (fb.ShowDialog(this) == DialogResult.OK)
            {
                GdkPathTextBox.Text = fb.SelectedPath;
            }
        }
    }
}
