namespace minisphere.Gdk.DockPanes
{
    partial class InspectorPane
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(InspectorPane));
            this.LocalsView = new System.Windows.Forms.ListView();
            this.columnName = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnValue = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.imagesVarList = new System.Windows.Forms.ImageList(this.components);
            this.splitter = new System.Windows.Forms.SplitContainer();
            this.panel1 = new System.Windows.Forms.Panel();
            this.ExprTextBox = new System.Windows.Forms.TextBox();
            this.EvalButton = new System.Windows.Forms.Button();
            this.CallsView = new System.Windows.Forms.ListView();
            this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnHeader3 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            ((System.ComponentModel.ISupportInitialize)(this.splitter)).BeginInit();
            this.splitter.Panel1.SuspendLayout();
            this.splitter.Panel2.SuspendLayout();
            this.splitter.SuspendLayout();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // LocalsView
            // 
            this.LocalsView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnName,
            this.columnValue});
            this.LocalsView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.LocalsView.FullRowSelect = true;
            this.LocalsView.GridLines = true;
            this.LocalsView.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.LocalsView.HideSelection = false;
            this.LocalsView.Location = new System.Drawing.Point(0, 0);
            this.LocalsView.MultiSelect = false;
            this.LocalsView.Name = "LocalsView";
            this.LocalsView.Size = new System.Drawing.Size(387, 333);
            this.LocalsView.SmallImageList = this.imagesVarList;
            this.LocalsView.TabIndex = 0;
            this.LocalsView.UseCompatibleStateImageBehavior = false;
            this.LocalsView.View = System.Windows.Forms.View.Details;
            this.LocalsView.DoubleClick += new System.EventHandler(this.LocalsView_DoubleClick);
            // 
            // columnName
            // 
            this.columnName.Text = "Name";
            this.columnName.Width = 125;
            // 
            // columnValue
            // 
            this.columnValue.Text = "Value";
            this.columnValue.Width = 200;
            // 
            // imagesVarList
            // 
            this.imagesVarList.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("imagesVarList.ImageStream")));
            this.imagesVarList.TransparentColor = System.Drawing.Color.Transparent;
            this.imagesVarList.Images.SetKeyName(0, "eye.png");
            // 
            // splitter
            // 
            this.splitter.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitter.Location = new System.Drawing.Point(0, 0);
            this.splitter.Name = "splitter";
            this.splitter.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitter.Panel1
            // 
            this.splitter.Panel1.Controls.Add(this.panel1);
            this.splitter.Panel1.Controls.Add(this.LocalsView);
            // 
            // splitter.Panel2
            // 
            this.splitter.Panel2.Controls.Add(this.CallsView);
            this.splitter.Size = new System.Drawing.Size(387, 654);
            this.splitter.SplitterDistance = 333;
            this.splitter.SplitterWidth = 3;
            this.splitter.TabIndex = 1;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.ExprTextBox);
            this.panel1.Controls.Add(this.EvalButton);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(0, 310);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(387, 23);
            this.panel1.TabIndex = 3;
            // 
            // ExprTextBox
            // 
            this.ExprTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.ExprTextBox.Font = new System.Drawing.Font("Consolas", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.ExprTextBox.Location = new System.Drawing.Point(0, 0);
            this.ExprTextBox.Name = "ExprTextBox";
            this.ExprTextBox.Size = new System.Drawing.Size(323, 23);
            this.ExprTextBox.TabIndex = 1;
            this.ExprTextBox.TextChanged += new System.EventHandler(this.ExprTextBox_TextChanged);
            this.ExprTextBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.ExprTextBox_KeyDown);
            // 
            // EvalButton
            // 
            this.EvalButton.Dock = System.Windows.Forms.DockStyle.Right;
            this.EvalButton.Enabled = false;
            this.EvalButton.Image = global::minisphere.Gdk.Properties.Resources.EvalIcon;
            this.EvalButton.Location = new System.Drawing.Point(323, 0);
            this.EvalButton.Name = "EvalButton";
            this.EvalButton.Size = new System.Drawing.Size(64, 23);
            this.EvalButton.TabIndex = 2;
            this.EvalButton.Text = "&Eval";
            this.EvalButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.EvalButton.UseVisualStyleBackColor = true;
            this.EvalButton.Click += new System.EventHandler(this.EvalButton_Click);
            // 
            // CallsView
            // 
            this.CallsView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2,
            this.columnHeader3});
            this.CallsView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.CallsView.FullRowSelect = true;
            this.CallsView.GridLines = true;
            this.CallsView.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.CallsView.HideSelection = false;
            this.CallsView.Location = new System.Drawing.Point(0, 0);
            this.CallsView.MultiSelect = false;
            this.CallsView.Name = "CallsView";
            this.CallsView.ShowItemToolTips = true;
            this.CallsView.Size = new System.Drawing.Size(387, 318);
            this.CallsView.TabIndex = 1;
            this.CallsView.UseCompatibleStateImageBehavior = false;
            this.CallsView.View = System.Windows.Forms.View.Details;
            this.CallsView.DoubleClick += new System.EventHandler(this.CallsView_DoubleClick);
            // 
            // columnHeader1
            // 
            this.columnHeader1.Text = "Function";
            this.columnHeader1.Width = 125;
            // 
            // columnHeader2
            // 
            this.columnHeader2.DisplayIndex = 2;
            this.columnHeader2.Text = "Script";
            this.columnHeader2.Width = 175;
            // 
            // columnHeader3
            // 
            this.columnHeader3.DisplayIndex = 1;
            this.columnHeader3.Text = "Line";
            this.columnHeader3.TextAlign = System.Windows.Forms.HorizontalAlignment.Right;
            this.columnHeader3.Width = 40;
            // 
            // InspectorPane
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.splitter);
            this.Font = new System.Drawing.Font("Segoe UI", 9F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "InspectorPane";
            this.Size = new System.Drawing.Size(387, 654);
            this.splitter.Panel1.ResumeLayout(false);
            this.splitter.Panel2.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.splitter)).EndInit();
            this.splitter.ResumeLayout(false);
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ListView LocalsView;
        private System.Windows.Forms.ColumnHeader columnName;
        private System.Windows.Forms.ColumnHeader columnValue;
        private System.Windows.Forms.SplitContainer splitter;
        private System.Windows.Forms.ImageList imagesVarList;
        private System.Windows.Forms.TextBox ExprTextBox;
        private System.Windows.Forms.Button EvalButton;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.ListView CallsView;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.ColumnHeader columnHeader3;
    }
}
