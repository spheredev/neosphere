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
            this.listVariables = new System.Windows.Forms.ListView();
            this.columnName = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.columnValue = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.imagesVarList = new System.Windows.Forms.ImageList(this.components);
            this.splitter = new System.Windows.Forms.SplitContainer();
            this.textValue = new System.Windows.Forms.TextBox();
            this.panelEval = new System.Windows.Forms.Panel();
            this.textEvalBox = new System.Windows.Forms.TextBox();
            this.buttonEval = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.splitter)).BeginInit();
            this.splitter.Panel1.SuspendLayout();
            this.splitter.Panel2.SuspendLayout();
            this.splitter.SuspendLayout();
            this.panelEval.SuspendLayout();
            this.SuspendLayout();
            // 
            // listVariables
            // 
            this.listVariables.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnName,
            this.columnValue});
            this.listVariables.Dock = System.Windows.Forms.DockStyle.Fill;
            this.listVariables.FullRowSelect = true;
            this.listVariables.GridLines = true;
            this.listVariables.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.listVariables.HideSelection = false;
            this.listVariables.Location = new System.Drawing.Point(0, 0);
            this.listVariables.MultiSelect = false;
            this.listVariables.Name = "listVariables";
            this.listVariables.Size = new System.Drawing.Size(332, 182);
            this.listVariables.SmallImageList = this.imagesVarList;
            this.listVariables.TabIndex = 0;
            this.listVariables.UseCompatibleStateImageBehavior = false;
            this.listVariables.View = System.Windows.Forms.View.Details;
            this.listVariables.SelectedIndexChanged += new System.EventHandler(this.listVariables_SelectedIndexChanged);
            // 
            // columnName
            // 
            this.columnName.Text = "Name";
            this.columnName.Width = 125;
            // 
            // columnValue
            // 
            this.columnValue.Text = "Value";
            this.columnValue.Width = 100;
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
            this.splitter.Panel1.Controls.Add(this.listVariables);
            // 
            // splitter.Panel2
            // 
            this.splitter.Panel2.Controls.Add(this.textValue);
            this.splitter.Panel2.Controls.Add(this.panelEval);
            this.splitter.Size = new System.Drawing.Size(332, 567);
            this.splitter.SplitterDistance = 182;
            this.splitter.SplitterWidth = 3;
            this.splitter.TabIndex = 1;
            // 
            // textValue
            // 
            this.textValue.Dock = System.Windows.Forms.DockStyle.Fill;
            this.textValue.Font = new System.Drawing.Font("Consolas", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.textValue.Location = new System.Drawing.Point(0, 21);
            this.textValue.Multiline = true;
            this.textValue.Name = "textValue";
            this.textValue.ReadOnly = true;
            this.textValue.ScrollBars = System.Windows.Forms.ScrollBars.Both;
            this.textValue.Size = new System.Drawing.Size(332, 361);
            this.textValue.TabIndex = 0;
            this.textValue.WordWrap = false;
            // 
            // panelEval
            // 
            this.panelEval.Controls.Add(this.textEvalBox);
            this.panelEval.Controls.Add(this.buttonEval);
            this.panelEval.Dock = System.Windows.Forms.DockStyle.Top;
            this.panelEval.Location = new System.Drawing.Point(0, 0);
            this.panelEval.Name = "panelEval";
            this.panelEval.Size = new System.Drawing.Size(332, 21);
            this.panelEval.TabIndex = 2;
            // 
            // textEvalBox
            // 
            this.textEvalBox.Dock = System.Windows.Forms.DockStyle.Fill;
            this.textEvalBox.Font = new System.Drawing.Font("Consolas", 9.75F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.textEvalBox.Location = new System.Drawing.Point(0, 0);
            this.textEvalBox.Name = "textEvalBox";
            this.textEvalBox.Size = new System.Drawing.Size(277, 23);
            this.textEvalBox.TabIndex = 1;
            this.textEvalBox.TextChanged += new System.EventHandler(this.textEvalBox_TextChanged);
            this.textEvalBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.textEvalBox_KeyDown);
            // 
            // buttonEval
            // 
            this.buttonEval.Dock = System.Windows.Forms.DockStyle.Right;
            this.buttonEval.Enabled = false;
            this.buttonEval.Image = global::minisphere.Gdk.Properties.Resources.EvalIcon;
            this.buttonEval.Location = new System.Drawing.Point(277, 0);
            this.buttonEval.Name = "buttonEval";
            this.buttonEval.Size = new System.Drawing.Size(55, 21);
            this.buttonEval.TabIndex = 2;
            this.buttonEval.Text = "&Eval";
            this.buttonEval.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.buttonEval.UseVisualStyleBackColor = true;
            this.buttonEval.Click += new System.EventHandler(this.buttonEval_Click);
            // 
            // InspectorPane
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.splitter);
            this.Font = new System.Drawing.Font("Segoe UI", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "InspectorPane";
            this.Size = new System.Drawing.Size(332, 567);
            this.splitter.Panel1.ResumeLayout(false);
            this.splitter.Panel2.ResumeLayout(false);
            this.splitter.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitter)).EndInit();
            this.splitter.ResumeLayout(false);
            this.panelEval.ResumeLayout(false);
            this.panelEval.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.ListView listVariables;
        private System.Windows.Forms.ColumnHeader columnName;
        private System.Windows.Forms.ColumnHeader columnValue;
        private System.Windows.Forms.SplitContainer splitter;
        private System.Windows.Forms.TextBox textValue;
        private System.Windows.Forms.ImageList imagesVarList;
        private System.Windows.Forms.TextBox textEvalBox;
        private System.Windows.Forms.Panel panelEval;
        private System.Windows.Forms.Button buttonEval;
    }
}
