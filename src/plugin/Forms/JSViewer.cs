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

using minisphere.Gdk.Duktape;
using minisphere.Gdk.Plugins;

namespace minisphere.Gdk.Forms
{
    partial class JSViewer : Form
    {
        private Inferior _inferior;
        private HeapPtr _objectPtr;

        public JSViewer(Inferior inferior, HeapPtr objectPtr)
        {
            InitializeComponent();

            _inferior = inferior;
            _objectPtr = objectPtr;
        }

        private async void JSViewer_Load(object sender, EventArgs e)
        {
            PropTree.BeginUpdate();
            PropTree.Nodes.Clear();
            var trunkName = _objectPtr.Class == ObjClass.Array ? "[ ... ]"
                : string.Format(@"{{ obj: '{0}' }}", _objectPtr.Class.ToString());
            var trunk = PropTree.Nodes.Add(trunkName);
            await PopulateTreeNode(trunk, _objectPtr);
            trunk.Expand();
            PropTree.EndUpdate();
        }

        private void JSViewer_Shown(object sender, EventArgs e)
        {
        }

        private async void PropTree_BeforeExpand(object sender, TreeViewCancelEventArgs e)
        {
            if (e.Node.Tag is HeapPtr)
                await PopulateTreeNode(e.Node, (HeapPtr)e.Node.Tag);
        }

        private async Task PopulateTreeNode(TreeNode node, HeapPtr ptr)
        {
            PropTree.BeginUpdate();
            var props = await _inferior.GetObjPropRange(ptr, 0, int.MaxValue);
            foreach (var key in props.Keys)
            {
                dynamic value = props[key];
                string friendlyValue =
                    value is HeapPtr && value.Class == ObjClass.Array ? "[ ... ]"
                    : value is HeapPtr ? string.Format(@"{{ obj: '{0}' }}", value.Class.ToString())
                    : value.Equals(DValue.Unused) ? "undefined"
                    : value.Equals(DValue.Undefined) ? "undefined"
                    : value.Equals(DValue.Null) ? "null"
                    : value is bool ? value ? "true" : "false"
                    : value is int ? value.ToString()
                    : value is double ? value.ToString()
                    : value is string ? string.Format("\"{0}\"", value.Replace(@"""", @"\""").Replace("\n", @"\n"))
                    : "*munch*";
                var nodeText = string.Format("{0} = {1}", key, friendlyValue);
                var valueNode = node.Nodes.Add(nodeText);
                if (value is HeapPtr)
                {
                    valueNode.Nodes.Add("");
                    valueNode.Tag = value;
                }
            }
            if (node.Tag is HeapPtr)
                node.Nodes.RemoveAt(0);
            node.Tag = null;
            PropTree.EndUpdate();
        }
    }
}
