using System;
using System.Collections.Generic;
using System.Drawing;
using System.Media;
using System.Threading.Tasks;
using System.Windows.Forms;

using Sphere.Plugins;
using Sphere.Plugins.Interfaces;
using Sphere.Plugins.Views;
using minisphere.Gdk.Debugger;
using minisphere.Gdk.Forms;
using minisphere.Gdk.Properties;

namespace minisphere.Gdk.DockPanes
{
    partial class InspectorPane : UserControl, IDockPane
    {
        private const string ValueBoxHint = "Select a variable from the list above to see what it contains.";

        private bool isEvaluating = false;
        private IReadOnlyDictionary<string, string> variables;

        public InspectorPane()
        {
            InitializeComponent();
            Enabled = false;
        }

        public bool ShowInViewMenu { get { return false; } }
        public Control Control { get { return this; } }
        public DockHint DockHint { get { return DockHint.Right; } }
        public Bitmap DockIcon { get { return Resources.InspectorIcon; } }

        public DebugSession Session { get; set; }

        public void Clear()
        {
            LocalsView.Items.Clear();
            CallsView.Items.Clear();
        }

        public void SetCallStack(Tuple<string, string, int>[] stack)
        {
            CallsView.BeginUpdate();
            CallsView.Items.Clear();
            foreach (var entry in stack)
            {
                ListViewItem item = new ListViewItem(entry.Item1 != ""
                    ? string.Format("{0}()", entry.Item1)
                    : "function()");
                item.SubItems.Add(entry.Item2);
                item.SubItems.Add(entry.Item3.ToString());
                CallsView.Items.Add(item);
            }
            CallsView.EndUpdate();
        }

        public void SetVariables(IReadOnlyDictionary<string, string> variables)
        {
            this.variables = variables;
            LocalsView.BeginUpdate();
            Clear();
            foreach (var k in this.variables.Keys)
            {
                var item = LocalsView.Items.Add(k, 0);
                item.SubItems.Add(this.variables[k]);
            }
            LocalsView.EndUpdate();
        }

        private async Task DoEvaluate(string expr)
        {
            isEvaluating = true;
            ExprTextBox.Text = expr;
            ExprTextBox.Enabled = false;
            EvalButton.Enabled = false;
            string value = await Session.Evaluate(expr);
            isEvaluating = false;
            new JSViewer(expr, value).ShowDialog(this);
            ExprTextBox.Text = "";
            ExprTextBox.Enabled = true;
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
                && !isEvaluating;
        }

        private void CallsView_DoubleClick(object sender, EventArgs e)
        {
            if (CallsView.SelectedItems.Count > 0)
            {
                ListViewItem item = CallsView.SelectedItems[0];
                string filename = Session.ResolvePath(item.SubItems[1].Text);
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

        private void LocalsView_DoubleClick(object sender, EventArgs e)
        {
            if (LocalsView.SelectedItems.Count > 0)
            {
                ListViewItem item = LocalsView.SelectedItems[0];
                ExprTextBox.Text = item.Text;
                EvalButton.PerformClick();
            }
        }
    }
}
