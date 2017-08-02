namespace Sphere.Gdk.DockPanes
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
            this.m_localsListView = new System.Windows.Forms.ListView();
            this.columnName = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnValue = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.imagesVarList = new System.Windows.Forms.ImageList(this.components);
            this.splitter = new System.Windows.Forms.SplitContainer();
            this.panel1 = new System.Windows.Forms.Panel();
            this.m_evalTextBox = new System.Windows.Forms.TextBox();
            this.m_evalButton = new System.Windows.Forms.Button();
            this.m_callsListView = new System.Windows.Forms.ListView();
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
            // m_localsListView
            // 
            this.m_localsListView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnName,
            this.columnValue});
            this.m_localsListView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.m_localsListView.FullRowSelect = true;
            this.m_localsListView.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.m_localsListView.HideSelection = false;
            this.m_localsListView.Location = new System.Drawing.Point(0, 0);
            this.m_localsListView.MultiSelect = false;
            this.m_localsListView.Name = "m_localsListView";
            this.m_localsListView.Size = new System.Drawing.Size(387, 333);
            this.m_localsListView.SmallImageList = this.imagesVarList;
            this.m_localsListView.Sorting = System.Windows.Forms.SortOrder.Ascending;
            this.m_localsListView.TabIndex = 0;
            this.m_localsListView.UseCompatibleStateImageBehavior = false;
            this.m_localsListView.View = System.Windows.Forms.View.Details;
            this.m_localsListView.DoubleClick += new System.EventHandler(this.LocalsView_DoubleClick);
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
            this.splitter.Panel1.Controls.Add(this.m_localsListView);
            // 
            // splitter.Panel2
            // 
            this.splitter.Panel2.Controls.Add(this.m_callsListView);
            this.splitter.Size = new System.Drawing.Size(387, 654);
            this.splitter.SplitterDistance = 333;
            this.splitter.SplitterWidth = 3;
            this.splitter.TabIndex = 1;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.m_evalTextBox);
            this.panel1.Controls.Add(this.m_evalButton);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
            this.panel1.Location = new System.Drawing.Point(0, 310);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(387, 23);
            this.panel1.TabIndex = 3;
            // 
            // m_evalTextBox
            // 
            this.m_evalTextBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.m_evalTextBox.Font = new System.Drawing.Font("Consolas", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.m_evalTextBox.Location = new System.Drawing.Point(0, 0);
            this.m_evalTextBox.Name = "m_evalTextBox";
            this.m_evalTextBox.Size = new System.Drawing.Size(323, 23);
            this.m_evalTextBox.TabIndex = 1;
            this.m_evalTextBox.TextChanged += new System.EventHandler(this.ExprTextBox_TextChanged);
            this.m_evalTextBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.ExprTextBox_KeyDown);
            // 
            // m_evalButton
            // 
            this.m_evalButton.Dock = System.Windows.Forms.DockStyle.Right;
            this.m_evalButton.Enabled = false;
            this.m_evalButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.m_evalButton.Image = global::Sphere.Gdk.Properties.Resources.EvalIcon;
            this.m_evalButton.Location = new System.Drawing.Point(323, 0);
            this.m_evalButton.Name = "m_evalButton";
            this.m_evalButton.Size = new System.Drawing.Size(64, 23);
            this.m_evalButton.TabIndex = 2;
            this.m_evalButton.Text = "&Eval";
            this.m_evalButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.m_evalButton.UseVisualStyleBackColor = true;
            this.m_evalButton.Click += new System.EventHandler(this.EvalButton_Click);
            // 
            // m_callsListView
            // 
            this.m_callsListView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2,
            this.columnHeader3});
            this.m_callsListView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.m_callsListView.FullRowSelect = true;
            this.m_callsListView.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.m_callsListView.HideSelection = false;
            this.m_callsListView.Location = new System.Drawing.Point(0, 0);
            this.m_callsListView.MultiSelect = false;
            this.m_callsListView.Name = "m_callsListView";
            this.m_callsListView.ShowItemToolTips = true;
            this.m_callsListView.Size = new System.Drawing.Size(387, 318);
            this.m_callsListView.TabIndex = 1;
            this.m_callsListView.UseCompatibleStateImageBehavior = false;
            this.m_callsListView.View = System.Windows.Forms.View.Details;
            this.m_callsListView.DoubleClick += new System.EventHandler(this.CallsView_DoubleClick);
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

        private System.Windows.Forms.ListView m_localsListView;
        private System.Windows.Forms.ColumnHeader columnName;
        private System.Windows.Forms.ColumnHeader columnValue;
        private System.Windows.Forms.SplitContainer splitter;
        private System.Windows.Forms.ImageList imagesVarList;
        private System.Windows.Forms.TextBox m_evalTextBox;
        private System.Windows.Forms.Button m_evalButton;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.ListView m_callsListView;
        private System.Windows.Forms.ColumnHeader columnHeader1;
        private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.ColumnHeader columnHeader3;
    }
}
