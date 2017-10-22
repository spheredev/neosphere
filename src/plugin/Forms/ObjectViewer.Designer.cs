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
            this.m_nameTextBox = new System.Windows.Forms.TextBox();
            this.SuspendLayout();
            // 
            // m_okButton
            // 
            this.m_okButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.m_okButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.m_okButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.m_okButton.Location = new System.Drawing.Point(485, 388);
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
            this.m_propListTreeView.Size = new System.Drawing.Size(553, 341);
            this.m_propListTreeView.TabIndex = 0;
            this.m_propListTreeView.BeforeExpand += new System.Windows.Forms.TreeViewCancelEventHandler(this.PropTree_BeforeExpand);
            this.m_propListTreeView.MouseMove += new System.Windows.Forms.MouseEventHandler(this.PropTree_MouseMove);
            // 
            // TreeIconImageList
            // 
            this.TreeIconImageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
            this.TreeIconImageList.ImageSize = new System.Drawing.Size(16, 16);
            this.TreeIconImageList.TransparentColor = System.Drawing.Color.Transparent;
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
            this.ClientSize = new System.Drawing.Size(577, 425);
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
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Button m_okButton;
        private System.Windows.Forms.TreeView m_propListTreeView;
        private System.Windows.Forms.ImageList TreeIconImageList;
        private System.Windows.Forms.TextBox m_nameTextBox;
    }
}