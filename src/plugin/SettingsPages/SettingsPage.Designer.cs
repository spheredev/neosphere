namespace Sphere.Gdk.SettingsPages
{
    partial class SettingsPage
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
            this.TabView = new System.Windows.Forms.TabControl();
            this.MainTabPage = new System.Windows.Forms.TabPage();
            this.panel2 = new System.Windows.Forms.Panel();
            this.ShowTraceCheckBox = new System.Windows.Forms.CheckBox();
            this.TestInWindowCheckBox = new System.Windows.Forms.CheckBox();
            this.MakeDebugPackageCheckBox = new System.Windows.Forms.CheckBox();
            this.editorLabel3 = new SphereStudio.UI.DialogHeader();
            this.panel3 = new System.Windows.Forms.Panel();
            this.VerbosityComboBox = new System.Windows.Forms.ComboBox();
            this.label2 = new System.Windows.Forms.Label();
            this.TestWithConsoleCheckBox = new System.Windows.Forms.CheckBox();
            this.editorLabel2 = new SphereStudio.UI.DialogHeader();
            this.panel1 = new System.Windows.Forms.Panel();
            this.BrowseButton = new System.Windows.Forms.Button();
            this.GdkPathTextBox = new System.Windows.Forms.TextBox();
            this.PathLabel = new System.Windows.Forms.Label();
            this.editorLabel1 = new SphereStudio.UI.DialogHeader();
            this.TabView.SuspendLayout();
            this.MainTabPage.SuspendLayout();
            this.panel2.SuspendLayout();
            this.panel3.SuspendLayout();
            this.panel1.SuspendLayout();
            this.SuspendLayout();
            // 
            // TabView
            // 
            this.TabView.Controls.Add(this.MainTabPage);
            this.TabView.Dock = System.Windows.Forms.DockStyle.Fill;
            this.TabView.Location = new System.Drawing.Point(0, 0);
            this.TabView.Name = "TabView";
            this.TabView.SelectedIndex = 0;
            this.TabView.Size = new System.Drawing.Size(576, 355);
            this.TabView.TabIndex = 3;
            // 
            // MainTabPage
            // 
            this.MainTabPage.Controls.Add(this.panel2);
            this.MainTabPage.Controls.Add(this.editorLabel3);
            this.MainTabPage.Controls.Add(this.panel3);
            this.MainTabPage.Controls.Add(this.editorLabel2);
            this.MainTabPage.Controls.Add(this.panel1);
            this.MainTabPage.Controls.Add(this.editorLabel1);
            this.MainTabPage.Location = new System.Drawing.Point(4, 22);
            this.MainTabPage.Name = "MainTabPage";
            this.MainTabPage.Padding = new System.Windows.Forms.Padding(3);
            this.MainTabPage.Size = new System.Drawing.Size(568, 329);
            this.MainTabPage.TabIndex = 0;
            this.MainTabPage.Text = "Sphere GDK";
            this.MainTabPage.UseVisualStyleBackColor = true;
            // 
            // panel2
            // 
            this.panel2.Controls.Add(this.ShowTraceCheckBox);
            this.panel2.Controls.Add(this.TestInWindowCheckBox);
            this.panel2.Controls.Add(this.MakeDebugPackageCheckBox);
            this.panel2.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel2.Location = new System.Drawing.Point(3, 190);
            this.panel2.Name = "panel2";
            this.panel2.Size = new System.Drawing.Size(562, 136);
            this.panel2.TabIndex = 3;
            // 
            // ShowTraceCheckBox
            // 
            this.ShowTraceCheckBox.AutoSize = true;
            this.ShowTraceCheckBox.Location = new System.Drawing.Point(12, 58);
            this.ShowTraceCheckBox.Name = "ShowTraceCheckBox";
            this.ShowTraceCheckBox.Size = new System.Drawing.Size(278, 17);
            this.ShowTraceCheckBox.TabIndex = 4;
            this.ShowTraceCheckBox.Text = "Show output produced by \'SSj.trace\' while debugging";
            this.ShowTraceCheckBox.UseVisualStyleBackColor = true;
            // 
            // TestInWindowCheckBox
            // 
            this.TestInWindowCheckBox.AutoSize = true;
            this.TestInWindowCheckBox.Location = new System.Drawing.Point(12, 35);
            this.TestInWindowCheckBox.Name = "TestInWindowCheckBox";
            this.TestInWindowCheckBox.Size = new System.Drawing.Size(365, 17);
            this.TestInWindowCheckBox.TabIndex = 3;
            this.TestInWindowCheckBox.Text = "Force the engine to start in windowed mode when clicking \"Test Game\"";
            this.TestInWindowCheckBox.UseVisualStyleBackColor = true;
            // 
            // MakeDebugPackageCheckBox
            // 
            this.MakeDebugPackageCheckBox.AutoSize = true;
            this.MakeDebugPackageCheckBox.Location = new System.Drawing.Point(12, 12);
            this.MakeDebugPackageCheckBox.Name = "MakeDebugPackageCheckBox";
            this.MakeDebugPackageCheckBox.Size = new System.Drawing.Size(324, 17);
            this.MakeDebugPackageCheckBox.TabIndex = 2;
            this.MakeDebugPackageCheckBox.Text = "Include source maps when building an SPK package using Cell";
            this.MakeDebugPackageCheckBox.UseVisualStyleBackColor = true;
            // 
            // editorLabel3
            // 
            this.editorLabel3.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(64)))), ((int)(((byte)(64)))), ((int)(((byte)(64)))));
            this.editorLabel3.Dock = System.Windows.Forms.DockStyle.Top;
            this.editorLabel3.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.editorLabel3.ForeColor = System.Drawing.Color.White;
            this.editorLabel3.Location = new System.Drawing.Point(3, 167);
            this.editorLabel3.Name = "editorLabel3";
            this.editorLabel3.Size = new System.Drawing.Size(562, 23);
            this.editorLabel3.TabIndex = 4;
            this.editorLabel3.Text = "Debugging options";
            this.editorLabel3.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // panel3
            // 
            this.panel3.Controls.Add(this.VerbosityComboBox);
            this.panel3.Controls.Add(this.label2);
            this.panel3.Controls.Add(this.TestWithConsoleCheckBox);
            this.panel3.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel3.Location = new System.Drawing.Point(3, 126);
            this.panel3.Name = "panel3";
            this.panel3.Size = new System.Drawing.Size(562, 41);
            this.panel3.TabIndex = 5;
            // 
            // VerbosityComboBox
            // 
            this.VerbosityComboBox.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.VerbosityComboBox.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.VerbosityComboBox.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.VerbosityComboBox.FormattingEnabled = true;
            this.VerbosityComboBox.Items.AddRange(new object[] {
            "V0 - game output only",
            "V1 - basic logging",
            "V2 - high-level logging",
            "V3 - low-level logging",
            "V4 - log everything!"});
            this.VerbosityComboBox.Location = new System.Drawing.Point(401, 10);
            this.VerbosityComboBox.Name = "VerbosityComboBox";
            this.VerbosityComboBox.Size = new System.Drawing.Size(148, 21);
            this.VerbosityComboBox.TabIndex = 3;
            // 
            // label2
            // 
            this.label2.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(345, 13);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(50, 13);
            this.label2.TabIndex = 2;
            this.label2.Text = "Verbosity";
            // 
            // TestWithConsoleCheckBox
            // 
            this.TestWithConsoleCheckBox.AutoSize = true;
            this.TestWithConsoleCheckBox.Location = new System.Drawing.Point(12, 12);
            this.TestWithConsoleCheckBox.Name = "TestWithConsoleCheckBox";
            this.TestWithConsoleCheckBox.Size = new System.Drawing.Size(248, 17);
            this.TestWithConsoleCheckBox.TabIndex = 0;
            this.TestWithConsoleCheckBox.Text = "Use SpheRun to handle \'Test Game\' command";
            this.TestWithConsoleCheckBox.UseVisualStyleBackColor = true;
            // 
            // editorLabel2
            // 
            this.editorLabel2.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(64)))), ((int)(((byte)(64)))), ((int)(((byte)(64)))));
            this.editorLabel2.Dock = System.Windows.Forms.DockStyle.Top;
            this.editorLabel2.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.editorLabel2.ForeColor = System.Drawing.Color.White;
            this.editorLabel2.Location = new System.Drawing.Point(3, 103);
            this.editorLabel2.Name = "editorLabel2";
            this.editorLabel2.Size = new System.Drawing.Size(562, 23);
            this.editorLabel2.TabIndex = 2;
            this.editorLabel2.Text = "SpheRun-specific settings";
            this.editorLabel2.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.BrowseButton);
            this.panel1.Controls.Add(this.GdkPathTextBox);
            this.panel1.Controls.Add(this.PathLabel);
            this.panel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.panel1.Location = new System.Drawing.Point(3, 26);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(562, 77);
            this.panel1.TabIndex = 1;
            // 
            // BrowseButton
            // 
            this.BrowseButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
            this.BrowseButton.FlatStyle = System.Windows.Forms.FlatStyle.Flat;
            this.BrowseButton.Image = global::Sphere.Gdk.Properties.Resources.FolderIcon;
            this.BrowseButton.Location = new System.Drawing.Point(469, 39);
            this.BrowseButton.Name = "BrowseButton";
            this.BrowseButton.Size = new System.Drawing.Size(80, 25);
            this.BrowseButton.TabIndex = 4;
            this.BrowseButton.Text = "Browse...";
            this.BrowseButton.TextImageRelation = System.Windows.Forms.TextImageRelation.ImageBeforeText;
            this.BrowseButton.UseVisualStyleBackColor = true;
            this.BrowseButton.Click += new System.EventHandler(this.BrowseButton_Click);
            // 
            // GdkPathTextBox
            // 
            this.GdkPathTextBox.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
            this.GdkPathTextBox.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.GdkPathTextBox.Location = new System.Drawing.Point(64, 13);
            this.GdkPathTextBox.Name = "GdkPathTextBox";
            this.GdkPathTextBox.ReadOnly = true;
            this.GdkPathTextBox.Size = new System.Drawing.Size(485, 20);
            this.GdkPathTextBox.TabIndex = 2;
            // 
            // PathLabel
            // 
            this.PathLabel.AutoSize = true;
            this.PathLabel.Location = new System.Drawing.Point(9, 16);
            this.PathLabel.Name = "PathLabel";
            this.PathLabel.Size = new System.Drawing.Size(49, 13);
            this.PathLabel.TabIndex = 0;
            this.PathLabel.Text = "Directory";
            // 
            // editorLabel1
            // 
            this.editorLabel1.BackColor = System.Drawing.Color.FromArgb(((int)(((byte)(64)))), ((int)(((byte)(64)))), ((int)(((byte)(64)))));
            this.editorLabel1.Dock = System.Windows.Forms.DockStyle.Top;
            this.editorLabel1.Font = new System.Drawing.Font("Segoe UI", 9F);
            this.editorLabel1.ForeColor = System.Drawing.Color.White;
            this.editorLabel1.Location = new System.Drawing.Point(3, 3);
            this.editorLabel1.Name = "editorLabel1";
            this.editorLabel1.Size = new System.Drawing.Size(562, 23);
            this.editorLabel1.TabIndex = 0;
            this.editorLabel1.Text = "Where is Sphere installed?";
            this.editorLabel1.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
            // 
            // SettingsPage
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.Controls.Add(this.TabView);
            this.Name = "SettingsPage";
            this.Size = new System.Drawing.Size(576, 355);
            this.TabView.ResumeLayout(false);
            this.MainTabPage.ResumeLayout(false);
            this.panel2.ResumeLayout(false);
            this.panel2.PerformLayout();
            this.panel3.ResumeLayout(false);
            this.panel3.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.TabControl TabView;
        private System.Windows.Forms.TabPage MainTabPage;
        private System.Windows.Forms.Panel panel1;
        private System.Windows.Forms.Button BrowseButton;
        private System.Windows.Forms.TextBox GdkPathTextBox;
        private System.Windows.Forms.Label PathLabel;
        private SphereStudio.UI.DialogHeader editorLabel1;
        private System.Windows.Forms.Panel panel2;
        private System.Windows.Forms.CheckBox TestWithConsoleCheckBox;
        private SphereStudio.UI.DialogHeader editorLabel2;
        private System.Windows.Forms.CheckBox MakeDebugPackageCheckBox;
        private System.Windows.Forms.CheckBox TestInWindowCheckBox;
        private SphereStudio.UI.DialogHeader editorLabel3;
        private System.Windows.Forms.Panel panel3;
        private System.Windows.Forms.ComboBox VerbosityComboBox;
        private System.Windows.Forms.Label label2;
		private System.Windows.Forms.CheckBox ShowTraceCheckBox;
    }
}
