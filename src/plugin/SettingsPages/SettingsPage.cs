using System;
using System.Windows.Forms;

using SphereStudio.Base;

namespace Sphere.Gdk.SettingsPages
{
    partial class SettingsPage : UserControl, ISettingsPage, IStyleAware
    {
        private PluginMain m_main;

        public SettingsPage(PluginMain main)
        {
            InitializeComponent();
            StyleManager.AutoStyle(this);
            m_main = main;
        }

        public Control Control { get { return this; } }

        public void ApplyStyle(UIStyle style)
        {
            style.AsUIElement(MainTabPage);
            style.AsTextView(GdkPathTextBox);
            style.AsTextView(VerbosityComboBox);
            style.AsAccent(BrowseButton);
        }

        public bool Apply()
        {
            m_main.Conf.GdkPath = GdkPathTextBox.Text;
            m_main.Conf.MakeDebugPackages = MakeDebugPackageCheckBox.Checked;
            m_main.Conf.AlwaysUseConsole = TestWithConsoleCheckBox.Checked;
            m_main.Conf.ShowTraceInfo = ShowTraceCheckBox.Checked;
            m_main.Conf.TestInWindow = TestInWindowCheckBox.Checked;
            m_main.Conf.UseRetroMode = UseRetroModeCheckBox.Checked;
            m_main.Conf.Verbosity = VerbosityComboBox.SelectedIndex;
            return true;
        }

        protected override void OnLoad(EventArgs e)
        {
            GdkPathTextBox.Text = m_main.Conf.GdkPath;
            MakeDebugPackageCheckBox.Checked = m_main.Conf.MakeDebugPackages;
            ShowTraceCheckBox.Checked = m_main.Conf.ShowTraceInfo;
            TestWithConsoleCheckBox.Checked = m_main.Conf.AlwaysUseConsole;
            TestInWindowCheckBox.Checked = m_main.Conf.TestInWindow;
            UseRetroModeCheckBox.Checked = m_main.Conf.UseRetroMode;
            VerbosityComboBox.SelectedIndex = m_main.Conf.Verbosity;
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
