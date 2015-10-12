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
    partial class StackPane : UserControl, IDockPane
    {
        public StackPane()
        {
            InitializeComponent();
            Enabled = false;
        }

        public bool ShowInViewMenu { get { return false; } }
        public Control Control { get { return this; } }
        public DockHint DockHint { get { return DockHint.Right; } }
        public Bitmap DockIcon { get { return Resources.StackIcon; } }

        public DebugSession CurrentSession { get; set; }

        public void Clear()
        {
            listStack.Items.Clear();
        }

        public void UpdateStack(Tuple<string, string, int>[] stack)
        {
            listStack.BeginUpdate();
            listStack.Items.Clear();
            foreach (var entry in stack)
            {
                ListViewItem item = new ListViewItem(entry.Item1 != ""
                    ? string.Format("{0}()", entry.Item1)
                    : "function()");
                item.SubItems.Add(entry.Item2);
                item.SubItems.Add(entry.Item3.ToString());
                listStack.Items.Add(item);
            }
            listStack.EndUpdate();
        }

        private void listStack_DoubleClick(object sender, EventArgs e)
        {
            if (listStack.SelectedItems.Count > 0)
            {
                ListViewItem item = listStack.SelectedItems[0];
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
