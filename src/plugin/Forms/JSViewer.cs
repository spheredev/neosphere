using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace minisphere.Gdk.Forms
{
    public partial class JSViewer : Form
    {
        public JSViewer(string expr, string value)
        {
            InitializeComponent();
            JsonTextBox.Text = string.Format("Expression: {0}\r\n\r\nValue:\r\n{1}", expr,
                Regex.Replace(value, "\r?\n", "\r\n", RegexOptions.None));
            Text = string.Format("{0} - {1}", expr, Text);
        }

        private void JSViewer_Load(object sender, EventArgs e)
        {
        }

        private void JSViewer_Shown(object sender, EventArgs e)
        {
            JsonTextBox.DeselectAll();
        }
    }
}
