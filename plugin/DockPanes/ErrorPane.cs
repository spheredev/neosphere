using System;
using System.Drawing;
using System.Media;
using System.Windows.Forms;

using Sphere.Plugins;
using Sphere.Plugins.Interfaces;
using Sphere.Plugins.Views;
using minisphere.Gdk.Debugger;
using minisphere.Gdk.Properties;

namespace minisphere.Gdk.DockPanes
{
    partial class ErrorPane : UserControl, IDockPane
    {
        public ErrorPane()
        {
            InitializeComponent();
        }

        public bool ShowInViewMenu { get { return true; } }
        public Control Control { get { return this; } }
        public DockHint DockHint { get { return DockHint.Bottom; } }
        public Bitmap DockIcon { get { return Resources.ErrorIcon; } }

        public DebugSession CurrentSession { get; set; }

        public void Add(string value, bool isFatal, string filename, int line)
        {
            if (listErrors.Items.Count > 0)
            {
                listErrors.Items[0].BackColor = listErrors.BackColor;
                listErrors.Items[0].ForeColor = listErrors.ForeColor;
            }
            ListViewItem item = listErrors.Items.Insert(0, value, isFatal ? 1 : 0);
            item.SubItems.Add(filename);
            item.SubItems.Add(line.ToString());
            if (isFatal)
            {
                item.BackColor = Color.DarkRed;
                item.ForeColor = Color.Yellow;
            }
        }

        public void Clear()
        {
            listErrors.Items.Clear();
        }

        public void ClearHighlight()
        {
            if (listErrors.Items.Count > 0)
            {
                listErrors.Items[0].BackColor = listErrors.BackColor;
                listErrors.Items[0].ForeColor = listErrors.ForeColor;
            }
        }

        public void HideIfClean()
        {
            ClearHighlight();
            if (listErrors.Items.Count == 0)
            {
                PluginManager.Core.Docking.Hide(this);
            }
        }

        private void listErrors_DoubleClick(object sender, EventArgs e)
        {
            if (listErrors.SelectedItems.Count > 0)
            {
                ListViewItem item = listErrors.SelectedItems[0];
                string filename = CurrentSession.ResolvePath(item.SubItems[1].Text);
                int lineNumber = int.Parse(item.SubItems[2].Text);
                ScriptView view = PluginManager.Core.OpenFile(filename) as ScriptView;
                if (view != null)
                {
                    view.GoToLine(lineNumber);
                }
                else
                {
                    SystemSounds.Asterisk.Play();
                }
            }
        }
    }
}

