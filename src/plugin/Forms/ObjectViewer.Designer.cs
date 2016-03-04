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
            this.ObjectNameTextBox = new System.Windows.Forms.TextBox();
            this.TreeIconImageList = new System.Windows.Forms.ImageList(this.components);
            this.SuspendLayout();
            // 
            // OKButton
            // 
            this.OKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.OKButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.OKButton.Location = new System.Drawing.Point(481, 418);
            this.OKButton.Name = "OKButton";
            this.OKButton.Size = new System.Drawing.Size(80, 25);
            this.OKButton.TabIndex = 1;
            this.OKButton.Text = "&Close";
            this.OKButton.UseVisualStyleBackColor = true;
            // 
            // PropTree
            // 
            this.PropTree.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.PropTree.ImageIndex = 0;
            this.PropTree.ImageList = this.TreeIconImageList;
            this.PropTree.Location = new System.Drawing.Point(12, 41);
            this.PropTree.Name = "PropTree";
            this.PropTree.SelectedImageIndex = 0;
            this.PropTree.Size = new System.Drawing.Size(549, 371);
            this.PropTree.TabIndex = 2;
            this.PropTree.BeforeExpand += new System.Windows.Forms.TreeViewCancelEventHandler(this.PropTree_BeforeExpand);
            // 
            // ObjectNameTextBox
            // 
            this.ObjectNameTextBox.Location = new System.Drawing.Point(12, 12);
            this.ObjectNameTextBox.Name = "ObjectNameTextBox";
            this.ObjectNameTextBox.ReadOnly = true;
            this.ObjectNameTextBox.Size = new System.Drawing.Size(549, 23);
            this.ObjectNameTextBox.TabIndex = 3;
            // 
            // TreeIconImageList
            // 
            this.TreeIconImageList.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
            this.TreeIconImageList.ImageSize = new System.Drawing.Size(16, 16);
            this.TreeIconImageList.TransparentColor = System.Drawing.Color.Transparent;
            // 
            // ObjectViewer
            // 
            this.AcceptButton = this.OKButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.OKButton;
            this.ClientSize = new System.Drawing.Size(573, 455);
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
            this.Load += new System.EventHandler(this.JSViewer_Load);
            this.Shown += new System.EventHandler(this.JSViewer_Shown);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion
        private System.Windows.Forms.Button OKButton;
        private System.Windows.Forms.TreeView PropTree;
        private System.Windows.Forms.TextBox ObjectNameTextBox;
        private System.Windows.Forms.ImageList TreeIconImageList;
    }
}