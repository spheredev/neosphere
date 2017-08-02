using System;
using System.Collections.Generic;
using System.Drawing;
using System.Media;
using System.Threading.Tasks;
using System.Windows.Forms;

using SphereStudio;
using SphereStudio.Base;
using SphereStudio.UI;

using Sphere.Gdk.Components;
using Sphere.Gdk.Forms;
using Sphere.Gdk.Properties;
using Sphere.Gdk.Debugger;

namespace Sphere.Gdk.DockPanes
{
    partial class InspectorPane : UserControl, IDockPane, IStyleable
    {
        private const string ValueBoxHint = "Select a variable from the list above to see what it contains.";

        private bool _isEvaling = false;
        private int _frame = -1;
        private IReadOnlyDictionary<string, DValue> _vars;

        public InspectorPane()
        {
            InitializeComponent();
            Styler.AutoStyle(this);

            Enabled = false;
        }

        public bool ShowInViewMenu => false;
        public Control Control => this;
        public DockHint DockHint => DockHint.Right;
        public Bitmap DockIcon => Resources.VisibleIcon;

        public SsjDebugger SSj { get; set; }

        public void ApplyStyle(UIStyle style)
        {
            style.AsUIElement(this);
            style.AsTextView(m_localsListView);
            style.AsTextView(m_callsListView);
            style.AsTextView(m_evalTextBox);
            style.AsAccent(m_evalButton);
        }

        public void Clear()
        {
            m_localsListView.Items.Clear();
            m_callsListView.Items.Clear();
        }

        public async Task SetCallStack(StackFrameInfo[] stackFrames, int index = 0)
        {
            m_callsListView.BeginUpdate();
            m_callsListView.Items.Clear();
            foreach (var frame in stackFrames)
            {
                var item = new ListViewItem(frame.FunctionName != "" ? string.Format("{0}()", frame.FunctionName) : "anon");
                var lineNumberString = frame.LineNumber != 0 ? frame.LineNumber.ToString() : "";
                item.SubItems.Add(frame.FileName);
                item.SubItems.Add(lineNumberString);
                m_callsListView.Items.Add(item);
            }
            m_callsListView.EndUpdate();
            _frame = -1;

            await LoadStackFrame(index);
        }

        private async Task DoEvaluate(string expr)
        {
            var result = await SSj.Inferior.Eval(expr, -(_frame + 1));
            new ObjectViewer(SSj.Inferior, expr, result).ShowDialog(this);
        }

        private async Task LoadStackFrame(int callIndex)
        {
            if (_frame >= 0)
                m_callsListView.Items[_frame].ForeColor = SystemColors.WindowText;
            _frame = callIndex;
            m_callsListView.Items[_frame].ForeColor = Color.Blue;
            m_callsListView.SelectedItems.Clear();
            _vars = await SSj.Inferior.GetLocals(-(_frame + 1));
            m_localsListView.BeginUpdate();
            m_localsListView.Items.Clear();
            foreach (var k in _vars.Keys)
            {
                var item = m_localsListView.Items.Add(k, 0);
                item.SubItems.Add(_vars[k].ToString());
            }
            m_localsListView.EndUpdate();
        }

        private async void EvalButton_Click(object sender, EventArgs e)
        {
            await DoEvaluate(m_evalTextBox.Text);
        }

        private void ExprTextBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter && e.Modifiers == Keys.None)
            {
                m_evalButton.PerformClick();
                e.Handled = true;
                e.SuppressKeyPress = true;
            }
        }

        private void ExprTextBox_TextChanged(object sender, EventArgs e)
        {
            m_evalButton.Enabled = !string.IsNullOrWhiteSpace(m_evalTextBox.Text)
                && !_isEvaling;
        }

        private async void LocalsView_DoubleClick(object sender, EventArgs e)
        {
            if (m_localsListView.SelectedItems.Count > 0)
            {
                ListViewItem item = m_localsListView.SelectedItems[0];
                await DoEvaluate(item.Text);
            }
        }

        private async void CallsView_DoubleClick(object sender, EventArgs e)
        {
            if (m_callsListView.SelectedItems.Count > 0)
            {
                ListViewItem item = m_callsListView.SelectedItems[0];
                var filename = SSj.ResolvePath(item.SubItems[1].Text);
                int.TryParse(item.SubItems[2].Text, out int lineNumber);
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
