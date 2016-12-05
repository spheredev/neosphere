using System;
using System.Collections.Generic;
using System.Drawing;
using System.Media;
using System.Threading.Tasks;
using System.Windows.Forms;

using Sphere.Plugins;
using Sphere.Plugins.Interfaces;
using Sphere.Plugins.Views;
using minisphere.Gdk.Forms;
using minisphere.Gdk.Plugins;
using minisphere.Gdk.Properties;
using minisphere.Gdk.Debugger;

namespace minisphere.Gdk.DockPanes
{
    partial class InspectorPane : UserControl, IDockPane
    {
        private const string ValueBoxHint = "Select a variable from the list above to see what it contains.";

        private bool _isEvaling = false;
        private int _frame = -1;
        private IReadOnlyDictionary<string, DValue> _vars;

        public InspectorPane()
        {
            InitializeComponent();
            Enabled = false;
        }

        public bool ShowInViewMenu => false;
        public Control Control => this;
        public DockHint DockHint => DockHint.Right;
        public Bitmap DockIcon => Resources.VisibleIcon;

        public SsjDebugger Ssj { get; set; }

        public void Clear()
        {
            LocalsView.Items.Clear();
            CallsView.Items.Clear();
        }

        public async Task SetCallStack(Tuple<string, string, int>[] stack, int index = 0)
        {
            CallsView.BeginUpdate();
            CallsView.Items.Clear();
            foreach (var entry in stack)
            {
                ListViewItem item = new ListViewItem(entry.Item1 != ""
                    ? string.Format("{0}()", entry.Item1) : "anon");
                item.SubItems.Add(entry.Item2);
                item.SubItems.Add(entry.Item3.ToString());
                CallsView.Items.Add(item);
            }
            CallsView.EndUpdate();
            _frame = -1;

            await LoadStackFrame(index);
        }

        private async Task DoEvaluate(string expr)
        {
            var result = await Ssj.Inferior.Eval(expr, -(_frame + 1));
            new ObjectViewer(Ssj.Inferior, expr, result).ShowDialog(this);
        }

        private async Task LoadStackFrame(int callIndex)
        {
            if (_frame >= 0)
                CallsView.Items[_frame].ForeColor = SystemColors.WindowText;
            _frame = callIndex;
            CallsView.Items[_frame].ForeColor = Color.Blue;
            CallsView.SelectedItems.Clear();
            _vars = await Ssj.Inferior.GetLocals(-(_frame + 1));
            LocalsView.BeginUpdate();
            LocalsView.Items.Clear();
            foreach (var k in _vars.Keys)
            {
                var item = LocalsView.Items.Add(k, 0);
                item.SubItems.Add(_vars[k].ToString());
            }
            LocalsView.EndUpdate();
        }

        private async void EvalButton_Click(object sender, EventArgs e)
        {
            await DoEvaluate(ExprTextBox.Text);
        }

        private void ExprTextBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter && e.Modifiers == Keys.None)
            {
                EvalButton.PerformClick();
                e.Handled = true;
                e.SuppressKeyPress = true;
            }
        }

        private void ExprTextBox_TextChanged(object sender, EventArgs e)
        {
            EvalButton.Enabled = !string.IsNullOrWhiteSpace(ExprTextBox.Text)
                && !_isEvaling;
        }

        private async void LocalsView_DoubleClick(object sender, EventArgs e)
        {
            if (LocalsView.SelectedItems.Count > 0)
            {
                ListViewItem item = LocalsView.SelectedItems[0];
                await DoEvaluate(item.Text);
            }
        }

        private async void CallsView_DoubleClick(object sender, EventArgs e)
        {
            if (CallsView.SelectedItems.Count > 0)
            {
                ListViewItem item = CallsView.SelectedItems[0];
                string filename = Ssj.ResolvePath(item.SubItems[1].Text);
                int lineNumber = int.Parse(item.SubItems[2].Text);
                ScriptView view = PluginManager.Core.OpenFile(filename) as ScriptView;
                if (view == null)
                    SystemSounds.Hand.Play();
                else
                {
                    await LoadStackFrame(item.Index);
                    view.Activate();
                    view.GoToLine(lineNumber);
                }
            }
        }
    }
}
