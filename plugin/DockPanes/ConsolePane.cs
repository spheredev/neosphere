using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using Sphere.Plugins;
using Sphere.Plugins.Interfaces;
using minisphere.Gdk.Properties;

namespace minisphere.Gdk.DockPanes
{
    partial class ConsolePane : UserControl, IDockPane
    {
        List<string> _lines = new List<string>();

        public ConsolePane()
        {
            InitializeComponent();
        }

        public bool ShowInViewMenu { get { return true; } }
        public Control Control { get { return this; } }
        public DockHint DockHint { get { return DockHint.Right; } }
        public Bitmap DockIcon { get { return Resources.ConsoleIcon; } }

        public void Clear()
        {
            _lines.Clear();
            PrintTimer.Start();
        }

        public void Print(string text)
        {
            _lines.Add(text);
            PrintTimer.Start();
        }

        private void PrintTimer_Tick(object sender, EventArgs e)
        {
            PrintTimer.Stop();
            textOutput.Text = string.Join("\r\n", _lines) + "\r\n";
            textOutput.SelectionStart = textOutput.Text.Length;
            textOutput.SelectionLength = 0;
            textOutput.ScrollToCaret();
        }
    }
}
