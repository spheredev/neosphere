using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Sphere.Plugins;
using Sphere.Plugins.Interfaces;

namespace minisphere.Gdk.Plugins
{
    class CellCompiler : IPackager
    {
        private PluginMain _main;

        public CellCompiler(PluginMain main)
        {
            _main = main;
        }

        public string SaveFileFilters
        {
            get { return "Sphere Game Package|*.spk"; }
        }

        public bool Prep(IProject project, IConsole con)
        {
            con.Print("Installing project template... ");
            CopyDirectory(Path.Combine(_main.Conf.GdkPath, "template"), project.RootPath);
            con.Print("OK.\n");

            con.Print("Generating Cellscript.js... ");
            var cellTemplatePath = Path.Combine(project.RootPath, "Cellscript.js.tmpl");
            try
            {
                var scriptPath = Path.Combine(project.RootPath, "Cellscript.js");
                var template = File.ReadAllText(cellTemplatePath);
                var script = string.Format(template,
                    JSifyString(project.Name, '"'), JSifyString(project.Author, '"'),
                    JSifyString(project.Summary, '"'),
                    project.ScreenWidth, project.ScreenHeight);
                File.WriteAllText(scriptPath, script);
                File.Delete(cellTemplatePath);
            }
            catch (Exception exc)
            {
                con.Print(string.Format("\n[error] {0}\n", exc.Message));
                return false;
            }
            con.Print("OK.\n");

            con.Print("Success!\n");
            return true;
        }

        public async Task<bool> Build(IProject project, string outPath, IConsole con)
        {
            string cellOptions = string.Format(@"--in-dir ""{0}"" --out-dir ""{1}"" --debug",
                project.RootPath.Replace(Path.DirectorySeparatorChar, '/'),
                outPath.Replace(Path.DirectorySeparatorChar, '/'));
            return await RunCell(cellOptions, con);
        }

        public async Task<bool> Package(IProject project, string fileName, IConsole con)
        {
            string cellOptions = string.Format(@"--in-dir ""{0}"" --out-dir ""{0}/.staging"" --package ""{1}""",
                project.RootPath.Replace(Path.DirectorySeparatorChar, '/'),
                fileName.Replace(Path.DirectorySeparatorChar, '/'));
            if (_main.Conf.MakeDebugPackages)
                cellOptions += " --debug";
            return await RunCell(cellOptions, con);
        }

        private void CopyDirectory(string sourcePath, string destPath)
        {
            var source = new DirectoryInfo(sourcePath);
            var target = new DirectoryInfo(destPath);
            target.Create();
            foreach (var fileInfo in source.GetFiles())
            {
                var destFileName = Path.Combine(target.FullName, fileInfo.Name);
                File.Copy(fileInfo.FullName, destFileName, true);
            }
            foreach (var dirInfo in source.GetDirectories())
            {
                var destFileName = Path.Combine(target.FullName, dirInfo.Name);
                CopyDirectory(dirInfo.FullName, destFileName);
            }
        }

        private string JSifyString(string str, char quoteChar)
        {
            str = str
                .Replace("\n", @"\n").Replace("\r", @"\r")
                .Replace(@"\", @"\\");
            if (quoteChar == '"')
                return str.Replace(@"""", @"\""");
            else if (quoteChar == '\'')
                return str.Replace("'", @"\'");
            else
                return str;
        }

        private async Task<bool> RunCell(string options, IConsole con)
        {
            string cellPath = Path.Combine(_main.Conf.GdkPath, "cell.exe");
            if (!File.Exists(cellPath))
            {
                con.Print("ERROR: no 'cell' executable was found, did Gohan kill Cell already?\n");
                con.Print("       (Please check your GDK path in Settings Center.)\n");
                return false;
            }

            ProcessStartInfo psi = new ProcessStartInfo(cellPath, options);
            psi.UseShellExecute = false;
            psi.CreateNoWindow = true;
            psi.RedirectStandardOutput = true;
            psi.RedirectStandardError = true;
            Process proc = Process.Start(psi);
            await Task.Run(() =>
            {
                StreamReader stdout = proc.StandardOutput;
                StreamReader stderr = proc.StandardError;
                while (!stdout.EndOfStream || !stderr.EndOfStream)
                {
                    if (stderr.Peek() > 0) con.Print(stderr.ReadLine() + "\n");
                    if (stdout.Peek() > 0) con.Print(stdout.ReadLine() + "\n");
                }
            });
            return proc.ExitCode == 0;
        }
    }
}
