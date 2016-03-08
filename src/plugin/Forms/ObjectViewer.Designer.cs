namespace minisphere.Gdk.Forms
{
    partial class ObjectViewer
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.OKButton = new System.Windows.Forms.Button();
            this.PropTree = new System.Windows.Forms.TreeView();
            this.TreeIconImageList = new System.Windows.Forms.ImageList(this.components);
            this.ObjectNameTextBox = new System.Windows.Forms.TextBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.AccessorCheckBox = new System.Windows.Forms.CheckBox();
            this.ConfigurableCheckBox = new System.Windows.Forms.CheckBox();
            this.EnumerableCheckBox = new System.Windows.Forms.CheckBox();
            this.WritableCheckBox = new System.Windows.Forms.CheckBox();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // OKButton
            // 
            this.OKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.OKButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.OKButton.Location = new System.Drawing.Point(481, 476);
            this.OKButton.Name = "OKButton";
            this.OKButton.Size = new System.Drawing.Size(80, 25);
            this.OKButton.TabIndex = 2;
            this.OKButton.Text = "&Close";
            this.OKButton.UseVisualStyleBackColor = true;
            // 
            // PropTree
            // 
            this.PropTree.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.PropTree.HideSelection = false;
            this.PropTree.HotTracking = true;
            this.PropTree.ImageIndex = 0;
            this.PropTree.ImageList = this.TreeIconImageList;
            this.PropTree.Location = new System.Drawing.Point(12, 41);
            this.PropTree.Name = "PropTree";
            this.PropTree.SelectedImageIndex = 0;
            this.PropTree.Size = new System.Drawing.Size(549, 361);
            this.PropTree.TabIndex = 0;
            this.PropTree.BeforeExpand += new System.Windows.Forms.TreeViewCancelEventHandler(this.PropTree_BeforeExpand);
            this.PropTree.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.PropTree_AfterSelect);
            this.PropTree.MouseMove += new System.Windows.Forms.MouseEventHandler(this.PropTree_MouseMove);
            // 
            // TreeIconImageList
            // 
            this.TreeIconImageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
            this.TreeIconImageList.ImageSize = new System.Drawing.Size(16, 16);
            this.TreeIconImageList.TransparentColor = System.Drawing.Color.Transparent;
            // 
            // ObjectNameTextBox
            // 
            this.ObjectNameTextBox.Location = new System.Drawing.Point(12, 12);
            this.ObjectNameTextBox.Name = "ObjectNameTextBox";
            this.ObjectNameTextBox.ReadOnly = true;
            this.ObjectNameTextBox.Size = new System.Drawing.Size(549, 23);
            this.ObjectNameTextBox.TabIndex = 3;
            // 
            // groupBox1
            // 
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.Controls.Add(this.AccessorCheckBox);
            this.groupBox1.Controls.Add(this.ConfigurableCheckBox);
            this.groupBox1.Controls.Add(this.EnumerableCheckBox);
            this.groupBox1.Controls.Add(this.WritableCheckBox);
            this.groupBox1.Location = new System.Drawing.Point(12, 408);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(549, 62);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Property Descriptor";
            // 
            // AccessorCheckBox
            // 
            this.AccessorCheckBox.AutoCheck = false;
            this.AccessorCheckBox.AutoSize = true;
            this.AccessorCheckBox.Location = new System.Drawing.Point(410, 26);
            this.AccessorCheckBox.Name = "AccessorCheckBox";
            this.AccessorCheckBox.Size = new System.Drawing.Size(123, 19);
            this.AccessorCheckBox.TabIndex = 3;
            this.AccessorCheckBox.Text = "Accessor (Get/Set)";
            this.AccessorCheckBox.UseVisualStyleBackColor = true;
            // 
            // ConfigurableCheckBox
            // 
            this.ConfigurableCheckBox.AutoCheck = false;
            this.ConfigurableCheckBox.AutoSize = true;
            this.ConfigurableCheckBox.Location = new System.Drawing.Point(186, 26);
            this.ConfigurableCheckBox.Name = "ConfigurableCheckBox";
            this.ConfigurableCheckBox.Size = new System.Drawing.Size(95, 19);
            this.ConfigurableCheckBox.TabIndex = 2;
            this.ConfigurableCheckBox.Text = "Configurable";
            this.ConfigurableCheckBox.UseVisualStyleBackColor = true;
            // 
            // EnumerableCheckBox
            // 
            this.EnumerableCheckBox.AutoCheck = false;
            this.EnumerableCheckBox.AutoSize = true;
            this.EnumerableCheckBox.Location = new System.Drawing.Point(91, 26);
            this.EnumerableCheckBox.Name = "EnumerableCheckBox";
            this.EnumerableCheckBox.Size = new System.Drawing.Size(89, 19);
            this.EnumerableCheckBox.TabIndex = 1;
            this.EnumerableCheckBox.Text = "Enumerable";
            this.EnumerableCheckBox.UseVisualStyleBackColor = true;
            // 
            // WritableCheckBox
            // 
            this.WritableCheckBox.AutoCheck = false;
            this.WritableCheckBox.AutoSize = true;
            this.WritableCheckBox.Location = new System.Drawing.Point(15, 26);
            this.WritableCheckBox.Name = "WritableCheckBox";
            this.WritableCheckBox.Size = new System.Drawing.Size(70, 19);
            this.WritableCheckBox.TabIndex = 0;
            this.WritableCheckBox.Text = "Writable";
            this.WritableCheckBox.UseVisualStyleBackColor = true;
            // 
            // ObjectViewer
            // 
            this.AcceptButton = this.OKButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.OKButton;
            this.ClientSize = new System.Drawing.Size(573, 513);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.ObjectNameTextBox);
            this.Controls.Add(this.PropTree);
            this.Controls.Add(this.OKButton);
            this.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ObjectViewer";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "SSJ2 Object Viewer";
            this.Load += new System.EventHandler(this.this_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Button OKButton;
        private System.Windows.Forms.TreeView PropTree;
        private System.Windows.Forms.TextBox ObjectNameTextBox;
        private System.Windows.Forms.ImageList TreeIconImageList;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.CheckBox AccessorCheckBox;
        private System.Windows.Forms.CheckBox ConfigurableCheckBox;
        private System.Windows.Forms.CheckBox EnumerableCheckBox;
        private System.Windows.Forms.CheckBox WritableCheckBox;
    }
}