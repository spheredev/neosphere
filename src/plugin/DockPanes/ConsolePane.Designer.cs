namespace Sphere.Gdk.DockPanes
{
    partial class ConsolePane
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

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(ConsolePane));
            this.m_timer = new System.Windows.Forms.Timer(this.components);
            this.m_splitBox = new System.Windows.Forms.SplitContainer();
            this.m_textBox = new System.Windows.Forms.TextBox();
            this.m_errorListView = new System.Windows.Forms.ListView();
            this.columnValue = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnScript = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnLine = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.m_errorImageList = new System.Windows.Forms.ImageList(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.m_splitBox)).BeginInit();
            this.m_splitBox.Panel1.SuspendLayout();
            this.m_splitBox.Panel2.SuspendLayout();
            this.m_splitBox.SuspendLayout();
            this.SuspendLayout();
            // 
            // m_timer
            // 
            this.m_timer.Interval = 250;
            this.m_timer.Tick += new System.EventHandler(this.timer_Tick);
            // 
            // m_splitBox
            // 
            this.m_splitBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.m_splitBox.Location = new System.Drawing.Point(0, 0);
            this.m_splitBox.Name = "m_splitBox";
            // 
            // m_splitBox.Panel1
            // 
            this.m_splitBox.Panel1.Controls.Add(this.m_textBox);
            // 
            // m_splitBox.Panel2
            // 
            this.m_splitBox.Panel2.Controls.Add(this.m_errorListView);
            this.m_splitBox.Size = new System.Drawing.Size(764, 221);
            this.m_splitBox.SplitterDistance = 385;
            this.m_splitBox.TabIndex = 1;
            // 
            // m_textBox
            // 
            this.m_textBox.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.m_textBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.m_textBox.Font = new System.Drawing.Font("Consolas", 10F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.m_textBox.Location = new System.Drawing.Point(0, 0);
            this.m_textBox.Multiline = true;
            this.m_textBox.Name = "m_textBox";
            this.m_textBox.ReadOnly = true;
            this.m_textBox.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.m_textBox.Size = new System.Drawing.Size(385, 221);
            this.m_textBox.TabIndex = 1;
            this.m_textBox.WordWrap = false;
            // 
            // m_errorListView
            // 
            this.m_errorListView.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.m_errorListView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnValue,
            this.columnScript,
            this.columnLine});
            this.m_errorListView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.m_errorListView.FullRowSelect = true;
            this.m_errorListView.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.m_errorListView.Location = new System.Drawing.Point(0, 0);
            this.m_errorListView.Name = "m_errorListView";
            this.m_errorListView.Size = new System.Drawing.Size(375, 221);
            this.m_errorListView.SmallImageList = this.m_errorImageList;
            this.m_errorListView.TabIndex = 1;
            this.m_errorListView.UseCompatibleStateImageBehavior = false;
            this.m_errorListView.View = System.Windows.Forms.View.Details;
            this.m_errorListView.DoubleClick += new System.EventHandler(this.errorListView_DoubleClick);
            // 
            // columnValue
            // 
            this.columnValue.Text = "Thrown Value";
            this.columnValue.Width = 400;
            // 
            // columnScript
            // 
            this.columnScript.Text = "Script";
            this.columnScript.Width = 200;
            // 
            // columnLine
            // 
            this.columnLine.Text = "Line";
            this.columnLine.Width = 50;
            // 
            // m_errorImageList
            // 
            this.m_errorImageList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("m_errorImageList.ImageStream")));
            this.m_errorImageList.TransparentColor = System.Drawing.Color.Transparent;
            this.m_errorImageList.Images.SetKeyName(0, "check.png");
            this.m_errorImageList.Images.SetKeyName(1, "cross.png");
            // 
            // ConsolePane
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.m_splitBox);
            this.DoubleBuffered = true;
            this.Name = "ConsolePane";
            this.Size = new System.Drawing.Size(764, 221);
            this.m_splitBox.Panel1.ResumeLayout(false);
            this.m_splitBox.Panel1.PerformLayout();
            this.m_splitBox.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.m_splitBox)).EndInit();
            this.m_splitBox.ResumeLayout(false);
            this.ResumeLayout(false);

        }

        #endregion
        private System.Windows.Forms.Timer m_timer;
        private System.Windows.Forms.SplitContainer m_splitBox;
        private System.Windows.Forms.TextBox m_textBox;
        private System.Windows.Forms.ListView m_errorListView;
        private System.Windows.Forms.ColumnHeader columnValue;
        private System.Windows.Forms.ColumnHeader columnScript;
        private System.Windows.Forms.ColumnHeader columnLine;
        private System.Windows.Forms.ImageList m_errorImageList;
    }
}
