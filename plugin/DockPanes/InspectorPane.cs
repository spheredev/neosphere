using System;
using System.Collections.Generic;
using System.Drawing;
using System.Threading.Tasks;
using System.Windows.Forms;

using Sphere.Plugins;
using Sphere.Plugins.Interfaces;
using minisphere.Gdk.Debugger;
using minisphere.Gdk.Properties;

namespace minisphere.Gdk.DockPanes
{
    partial class InspectorPane : UserControl, IDockPane
    {
        private const string ValueBoxHint = "Select a variable from the list above to see what it contains.";

        private bool isEvaluating = false;
        private string lastVarName = null;
        private IReadOnlyDictionary<string, string> variables;

        public InspectorPane()
        {
            InitializeComponent();
            Enabled = false;

            textValue.Text = ValueBoxHint;
            textValue.WordWrap = true;
        }

        public bool ShowInViewMenu { get { return false; } }
        public Control Control { get { return this; } }
        public DockHint DockHint { get { return DockHint.Right; } }
        public Bitmap DockIcon { get { return Resources.InspectorIcon; } }

        public DebugSession CurrentSession { get; set; }

        public void Clear()
        {
            listVariables.Items.Clear();
            textEvalBox.Text = "";
            textValue.Text = ValueBoxHint;
            textValue.WordWrap = true;
        }

        public void SetVariables(IReadOnlyDictionary<string, string> variables)
        {
            this.variables = variables;
            listVariables.BeginUpdate();
            Clear();
            foreach (var k in this.variables.Keys)
            {
                var item = listVariables.Items.Add(k, 0);
                item.SubItems.Add(this.variables[k]);
            }
            if (lastVarName != null)
            {
                var toSelect = listVariables.FindItemWithText(lastVarName);
                if (toSelect != null)
                    toSelect.Selected = true;
            }
            listVariables.EndUpdate();
        }

        private async Task DoEvaluate(string expression)
        {
            isEvaluating = true;
            textEvalBox.Text = expression;
            textEvalBox.Enabled = false;
            buttonEval.Enabled = false;
            textValue.Text = "Evaluating...";
            textValue.WordWrap = false;
            string value = await CurrentSession.Evaluate(expression);
            textValue.Text = string.Format("Expression:\r\n{0}\r\n\r\nValue:\r\n{1}",
                expression, value.Replace("\n", "\r\n"));
            textEvalBox.Text = "";
            textEvalBox.Enabled = true;
            buttonEval.Enabled = true;
            isEvaluating = false;
        }

        private async void buttonEval_Click(object sender, EventArgs e)
        {
            await DoEvaluate(textEvalBox.Text);
        }

        private async void listVariables_SelectedIndexChanged(object sender, EventArgs e)
        {
            lastVarName = listVariables.SelectedItems.Count > 0
                ? listVariables.SelectedItems[0].Text : null;
            if (listVariables.SelectedItems.Count > 0)
                await DoEvaluate(listVariables.SelectedItems[0].Text);
            else
            {
                textEvalBox.Text = "";
                textValue.Text = ValueBoxHint;
                textValue.WordWrap = true;
            }
        }

        private void textEvalBox_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Enter && e.Modifiers == Keys.None)
            {
                buttonEval.PerformClick();
                e.Handled = true;
                e.SuppressKeyPress = true;
            }
        }

        private void textEvalBox_TextChanged(object sender, EventArgs e)
        {
            if (isEvaluating) return;

            listVariables.SelectedItems.Clear();
            buttonEval.Enabled = !string.IsNullOrWhiteSpace(textEvalBox.Text);
        }
    }
}
