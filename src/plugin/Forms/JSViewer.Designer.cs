namespace minisphere.Gdk.Forms
{
    partial class JSViewer
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
            this.OKButton = new System.Windows.Forms.Button();
            this.PropTree = new System.Windows.Forms.TreeView();
            this.SuspendLayout();
            // 
            // OKButton
            // 
            this.OKButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
            this.OKButton.DialogResult = System.Windows.Forms.DialogResult.OK;
            this.OKButton.Location = new System.Drawing.Point(481, 364);
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
            this.PropTree.Location = new System.Drawing.Point(12, 12);
            this.PropTree.Name = "PropTree";
            this.PropTree.Size = new System.Drawing.Size(549, 346);
            this.PropTree.TabIndex = 2;
            this.PropTree.BeforeExpand += new System.Windows.Forms.TreeViewCancelEventHandler(this.PropTree_BeforeExpand);
            // 
            // JSViewer
            // 
            this.AcceptButton = this.OKButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.CancelButton = this.OKButton;
            this.ClientSize = new System.Drawing.Size(573, 401);
            this.Controls.Add(this.PropTree);
            this.Controls.Add(this.OKButton);
            this.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "JSViewer";
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "JS Object Viewer";
            this.Load += new System.EventHandler(this.JSViewer_Load);
            this.Shown += new System.EventHandler(this.JSViewer_Shown);
            this.ResumeLayout(false);

        }

        #endregion
        private System.Windows.Forms.Button OKButton;
        private System.Windows.Forms.TreeView PropTree;
    }
}