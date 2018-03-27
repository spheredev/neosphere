using System;
using System.Collections.Generic;
using System.Drawing;
using System.Media;
using System.Windows.Forms;

using SphereStudio.Base;

using Sphere.Gdk.Components;
using Sphere.Gdk.Properties;

namespace Sphere.Gdk.DockPanes
{
    partial class ConsolePane : UserControl, IDockPane, IStyleAware
    {
        PluginConf m_config;
        List<string> m_lines = new List<string>();

        public ConsolePane(PluginConf config)
        {
            InitializeComponent();
            StyleManager.AutoStyle(this);

            m_config = config;
        }

        public bool ShowInViewMenu => true;
        public Control Control => this;
        public DockHint DockHint => DockHint.Bottom;
        public Bitmap DockIcon => Resources.ConsoleIcon;

        public SsjDebugger Ssj { get; set; }

        public void AddError(string value, bool isFatal, string filename, int line)
        {
            if (m_errorListView.Items.Count > 0) {
                m_errorListView.Items[0].BackColor = m_errorListView.BackColor;
                m_errorListView.Items[0].ForeColor = m_errorListView.ForeColor;
            }
            ListViewItem item = m_errorListView.Items.Insert(0, value, isFatal ? 1 : 0);
            item.SubItems.Add(filename);
            item.SubItems.Add(line.ToString());
            if (isFatal) {
                item.BackColor = Color.DarkRed;
                item.ForeColor = Color.Yellow;
            }
        }

        public void ApplyStyle(UIStyle style)
        {
            style.AsCodeView(m_textBox);
            style.AsTextView(m_errorListView);
        }

        public void ClearConsole()
        {
            m_lines.Clear();
            m_timer.Start();
        }

        public void ClearErrors()
        {
            m_errorListView.Items.Clear();
        }

        public void ClearErrorHighlight()
        {
            if (m_errorListView.Items.Count > 0) {
                m_errorListView.Items[0].BackColor = m_errorListView.BackColor;
                m_errorListView.Items[0].ForeColor = m_errorListView.ForeColor;
            }
        }

        public void HideIfClean()
        {
            ClearErrorHighlight();
            if (m_errorListView.Items.Count == 0) {
                PluginManager.Core.Docking.Hide(this);
            }
        }

        public void Print(string text)
        {
            m_lines.AddRange(text.Split(new[] { "\r\n", "\n" }, StringSplitOptions.None));
            m_timer.Start();
        }

        private void errorListView_DoubleClick(object sender, EventArgs e)
        {
            if (m_errorListView.SelectedItems.Count > 0) {
                ListViewItem item = m_errorListView.SelectedItems[0];
                string filename = Ssj.ResolvePath(item.SubItems[1].Text);
                int lineNumber = int.Parse(item.SubItems[2].Text);
                ScriptView view = PluginManager.Core.OpenFile(filename) as ScriptView;
                if (view == null)
                    SystemSounds.Asterisk.Play();
                else
                    view.GoToLine(lineNumber);
            }
        }

        private void timer_Tick(object sender, EventArgs e)
        {
            m_timer.Stop();
            m_textBox.Text = string.Join("\r\n", m_lines) + "\r\n";
            m_textBox.SelectionStart = m_textBox.Text.Length;
            m_textBox.SelectionLength = 0;
            m_textBox.ScrollToCaret();
        }
    }
}
