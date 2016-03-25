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

            ObjectNameTextBox.Text = string.Format("eval('{0}') = {1};",
                objectName.Replace(@"\", @"\\").Replace("'", @"\'").Replace("\n", @"\n").Replace("\r", @"\r"),
                value.ToString());
            TreeIconImageList.Images.Add("object", Resources.StackIcon);
            TreeIconImageList.Images.Add("prop", Resources.VisibleIcon);
            TreeIconImageList.Images.Add("hiddenProp", Resources.InvisibleIcon);

            _inferior = inferior;
            _value = value;
        }

        private async void this_Load(object sender, EventArgs e)
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

        private void PropTree_AfterSelect(object sender, TreeViewEventArgs e)
        {
            if (e.Node.Tag != null)
            {
                PropDesc desc = (PropDesc)e.Node.Tag;
                WritableCheckBox.Enabled = !desc.Flags.HasFlag(PropFlags.Accessor);
                EnumerableCheckBox.Enabled = true;
                ConfigurableCheckBox.Enabled = true;
                AccessorCheckBox.Enabled = true;
                WritableCheckBox.Checked = desc.Flags.HasFlag(PropFlags.Writable);
                EnumerableCheckBox.Checked = desc.Flags.HasFlag(PropFlags.Enumerable);
                ConfigurableCheckBox.Checked = desc.Flags.HasFlag(PropFlags.Configurable);
                AccessorCheckBox.Checked = desc.Flags.HasFlag(PropFlags.Accessor);
            }
            else {
                WritableCheckBox.Enabled = WritableCheckBox.Checked = false;
                EnumerableCheckBox.Enabled = EnumerableCheckBox.Checked = false;
                ConfigurableCheckBox.Enabled = ConfigurableCheckBox.Checked = false;
                AccessorCheckBox.Enabled = AccessorCheckBox.Enabled = false;
            }
        }

        private async void PropTree_BeforeExpand(object sender, TreeViewCancelEventArgs e)
        {
            if (e.Node.Tag == null || e.Node.Nodes[0].Text != "")
                return;

            PropDesc propDesc = (PropDesc)e.Node.Tag;
            if (propDesc.Value.Tag == DValueTag.HeapPtr)
                await PopulateTreeNode(e.Node, (HeapPtr)propDesc.Value);
        }

        private void PropTree_MouseMove(object sender, MouseEventArgs e)
        {
            var ht = PropTree.HitTest(e.Location);
            PropTree.Cursor = ht.Node != null && ht.Node.Bounds.Contains(e.Location)
                ? Cursors.Hand : Cursors.Default;
        }

        private async Task PopulateTreeNode(TreeNode node, HeapPtr ptr)
        {
            PropTree.BeginUpdate();
            var props = await _inferior.GetObjPropDescRange(ptr, 0, int.MaxValue);
            foreach (var key in props.Keys) {
                if (props[key].Flags.HasFlag(PropFlags.Accessor)) {
                    DValue getter = props[key].Getter;
                    DValue setter = props[key].Setter;
                    var nodeText = string.Format("{0} = get: {1}, set: {2}", key,
                        getter.ToString(), setter.ToString());
                    var valueNode = node.Nodes.Add(nodeText);
                    valueNode.ImageKey = props[key].Flags.HasFlag(PropFlags.Enumerable) ? "prop" : "hiddenProp";
                    valueNode.SelectedImageKey = valueNode.ImageKey;
                    valueNode.Tag = props[key];
                }
                else {
                    DValue value = props[key].Value;
                    var nodeText = string.Format("{0} = {1}", key, value.ToString());
                    var valueNode = node.Nodes.Add(nodeText);
                    valueNode.ImageKey = props[key].Flags.HasFlag(PropFlags.Enumerable) ? "prop" : "hiddenProp";
                    valueNode.SelectedImageKey = valueNode.ImageKey;
                    valueNode.Tag = props[key];
                    if (value.Tag == DValueTag.HeapPtr) {
                        valueNode.Nodes.Add("");
                    }
                }
            }
            if (node.Nodes.Count > 0 && node.Nodes[0].Text == "")
                node.Nodes.RemoveAt(0);
            PropTree.EndUpdate();
        }
    }
}
