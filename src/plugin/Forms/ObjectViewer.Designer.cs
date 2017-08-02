namespace Sphere.Gdk.Forms
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
            this.m_okButton = new System.Windows.Forms.Button();
            this.m_propListTreeView = new System.Windows.Forms.TreeView();
            this.TreeIconImageList = new System.Windows.Forms.ImageList(this.components);
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.m_accessorCheckBox = new System.Windows.Forms.CheckBox();
            this.m_configurableCheckBox = new System.Windows.Forms.CheckBox();
            this.m_enumerableCheckBox = new System.Windows.Forms.CheckBox();
            this.m_writableCheckBox = new System.Windows.Forms.CheckBox();
            this.m_nameTextBox = new System.Windows.Forms.TextBox();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // m_okButton
            // 
            this.m_okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.m_okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.m_okButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.m_okButton.Location = new System.Drawing.Point(485, 417);
            this.m_okButton.Name = "m_okButton";
            this.m_okButton.Size = new System.Drawing.Size(80, 25);
            this.m_okButton.TabIndex = 2;
            this.m_okButton.Text = "&Close";
            this.m_okButton.UseVisualStyleBackColor = true;
            // 
            // m_propListTreeView
            // 
            this.m_propListTreeView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.m_propListTreeView.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.m_propListTreeView.HideSelection = false;
            this.m_propListTreeView.HotTracking = true;
            this.m_propListTreeView.ImageIndex = 0;
            this.m_propListTreeView.ImageList = this.TreeIconImageList;
            this.m_propListTreeView.Location = new System.Drawing.Point(12, 41);
            this.m_propListTreeView.Name = "m_propListTreeView";
            this.m_propListTreeView.SelectedImageIndex = 0;
            this.m_propListTreeView.Size = new System.Drawing.Size(553, 302);
            this.m_propListTreeView.TabIndex = 0;
            this.m_propListTreeView.BeforeExpand += new System.Windows.Forms.TreeViewCancelEventHandler(this.PropTree_BeforeExpand);
            this.m_propListTreeView.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.PropTree_AfterSelect);
            this.m_propListTreeView.MouseMove += new System.Windows.Forms.MouseEventHandler(this.PropTree_MouseMove);
            // 
            // TreeIconImageList
            // 
            this.TreeIconImageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
            this.TreeIconImageList.ImageSize = new System.Drawing.Size(16, 16);
            this.TreeIconImageList.TransparentColor = System.Drawing.Color.Transparent;
            // 
            // groupBox1
            // 
            this.groupBox1.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.groupBox1.Controls.Add(this.m_accessorCheckBox);
            this.groupBox1.Controls.Add(this.m_configurableCheckBox);
            this.groupBox1.Controls.Add(this.m_enumerableCheckBox);
            this.groupBox1.Controls.Add(this.m_writableCheckBox);
            this.groupBox1.Location = new System.Drawing.Point(12, 349);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(553, 62);
            this.groupBox1.TabIndex = 1;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Property Descriptor";
            // 
            // m_accessorCheckBox
            // 
            this.m_accessorCheckBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.m_accessorCheckBox.AutoCheck = false;
            this.m_accessorCheckBox.AutoSize = true;
            this.m_accessorCheckBox.Location = new System.Drawing.Point(424, 26);
            this.m_accessorCheckBox.Name = "m_accessorCheckBox";
            this.m_accessorCheckBox.Size = new System.Drawing.Size(123, 19);
            this.m_accessorCheckBox.TabIndex = 3;
            this.m_accessorCheckBox.Text = "Accessor (Get/Set)";
            this.m_accessorCheckBox.UseVisualStyleBackColor = true;
            // 
            // m_configurableCheckBox
            // 
            this.m_configurableCheckBox.AutoCheck = false;
            this.m_configurableCheckBox.AutoSize = true;
            this.m_configurableCheckBox.Location = new System.Drawing.Point(186, 26);
            this.m_configurableCheckBox.Name = "m_configurableCheckBox";
            this.m_configurableCheckBox.Size = new System.Drawing.Size(95, 19);
            this.m_configurableCheckBox.TabIndex = 2;
            this.m_configurableCheckBox.Text = "Configurable";
            this.m_configurableCheckBox.UseVisualStyleBackColor = true;
            // 
            // m_enumerableCheckBox
            // 
            this.m_enumerableCheckBox.AutoCheck = false;
            this.m_enumerableCheckBox.AutoSize = true;
            this.m_enumerableCheckBox.Location = new System.Drawing.Point(91, 26);
            this.m_enumerableCheckBox.Name = "m_enumerableCheckBox";
            this.m_enumerableCheckBox.Size = new System.Drawing.Size(89, 19);
            this.m_enumerableCheckBox.TabIndex = 1;
            this.m_enumerableCheckBox.Text = "Enumerable";
            this.m_enumerableCheckBox.UseVisualStyleBackColor = true;
            // 
            // m_writableCheckBox
            // 
            this.m_writableCheckBox.AutoCheck = false;
            this.m_writableCheckBox.AutoSize = true;
            this.m_writableCheckBox.Location = new System.Drawing.Point(15, 26);
            this.m_writableCheckBox.Name = "m_writableCheckBox";
            this.m_writableCheckBox.Size = new System.Drawing.Size(70, 19);
            this.m_writableCheckBox.TabIndex = 0;
            this.m_writableCheckBox.Text = "Writable";
            this.m_writableCheckBox.UseVisualStyleBackColor = true;
            // 
            // m_nameTextBox
            // 
            this.m_nameTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.m_nameTextBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.m_nameTextBox.Location = new System.Drawing.Point(12, 12);
            this.m_nameTextBox.Name = "m_nameTextBox";
            this.m_nameTextBox.ReadOnly = true;
            this.m_nameTextBox.Size = new System.Drawing.Size(553, 23);
            this.m_nameTextBox.TabIndex = 3;
            // 
            // ObjectViewer
            // 
            this.AcceptButton = this.m_okButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.m_okButton;
            this.ClientSize = new System.Drawing.Size(577, 454);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.m_nameTextBox);
            this.Controls.Add(this.m_propListTreeView);
            this.Controls.Add(this.m_okButton);
            this.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "ObjectViewer";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "JavaScript Object Viewer";
            this.Load += new System.EventHandler(this.this_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Button m_okButton;
        private System.Windows.Forms.TreeView m_propListTreeView;
        private System.Windows.Forms.ImageList TreeIconImageList;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.CheckBox m_accessorCheckBox;
        private System.Windows.Forms.CheckBox m_configurableCheckBox;
        private System.Windows.Forms.CheckBox m_enumerableCheckBox;
        private System.Windows.Forms.CheckBox m_writableCheckBox;
        private System.Windows.Forms.TextBox m_nameTextBox;
    }
}