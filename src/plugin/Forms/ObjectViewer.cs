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
        private DValue _value;

        public ObjectViewer(Inferior inferior, string objectName, DValue value)
        {
            InitializeComponent();

            ObjectNameTextBox.Text = "eval('"
                + objectName.Replace(@"\", @"\\").Replace("'", @"\'").Replace("\n", @"\n")
                + "');";
            TreeIconImageList.Images.Add("object", Resources.StackIcon);
            TreeIconImageList.Images.Add("prop", Resources.VisibleIcon);
            TreeIconImageList.Images.Add("hiddenProp", Resources.InvisibleIcon);

            _inferior = inferior;
            _value = value;
        }

        private async void JSViewer_Load(object sender, EventArgs e)
        {
            PropTree.BeginUpdate();
            PropTree.Nodes.Clear();
            var trunk = PropTree.Nodes.Add(_value.ToString());
            trunk.ImageKey = "object";
            if (_value.Tag == DValueTag.HeapPtr)
                await PopulateTreeNode(trunk, (HeapPtr)_value);
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
                DValue value = props[key].Value;
                var nodeText = string.Format("{0} = {1}", key, value.ToString());
                var valueNode = node.Nodes.Add(nodeText);
                valueNode.ImageKey = props[key].Flags.HasFlag(PropFlags.Enumerable) ? "prop" : "hiddenProp";
                valueNode.SelectedImageKey = valueNode.ImageKey;
                if (value.Tag == DValueTag.HeapPtr)
                {
                    valueNode.Nodes.Add("");
                    valueNode.Tag = (HeapPtr)value;
                }
            }
            if (node.Tag is HeapPtr)
                node.Nodes.RemoveAt(0);
            node.Tag = null;
            PropTree.EndUpdate();
        }
    }
}
