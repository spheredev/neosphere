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

using minisphere.Gdk.Debugger;
using minisphere.Gdk.Plugins;
using minisphere.Gdk.Properties;

namespace minisphere.Gdk.Forms
{
    partial class ObjectViewer : Form
    {
        private Inferior _inferior;
        private HeapPtr _objectPtr;

        public ObjectViewer(Inferior inferior, string objectName, HeapPtr objectPtr)
        {
            InitializeComponent();

            ObjectNameTextBox.Text = objectName;
            TreeIconImageList.Images.Add("object", Resources.StackIcon);
            TreeIconImageList.Images.Add("prop", Resources.VisibleIcon);
            TreeIconImageList.Images.Add("hiddenProp", Resources.InvisibleIcon);

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
            trunk.ImageKey = "object";
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
                dynamic value = props[key].Value;
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
                valueNode.ImageKey = props[key].Flags.HasFlag(JSPropFlags.Enumerable) ? "prop" : "hiddenProp";
                valueNode.SelectedImageKey = valueNode.ImageKey;
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
