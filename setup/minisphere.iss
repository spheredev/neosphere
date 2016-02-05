; minisphere installer for Windows releases
; (C) 2015 Fat Cerberus
; Inno Setup 5.5 or later with ISPP is required.
;
; This will build a Windows installer for minisphere. Before compiling the installer,
; you must first build the complete minisphere GDK for both x86 and x64 using Visual Studio.
; In practical terms, this means building the full GDK and Redist solution
; configurations.


#define AppName "minisphere"
#define AppVersion "3.0a0"
#define AppRawVersion "3.0a0"
#define AppPublisher "Fat Cerberus"

[Setup]
OutputBaseFilename=minisphere-{#AppVersion}
OutputDir=.
AppId={{10C19C9F-1E29-45D8-A534-8FEF98C7C2FF}
AppName={#AppName}
AppVerName={#AppName} {#AppVersion}
AppVersion={#AppRawVersion}
AppPublisher=Fat Cerberus
AppUpdatesURL=http://forums.spheredev.org/index.php/topic,1215.0.html
AlwaysShowDirOnReadyPage=yes
ArchitecturesInstallIn64BitMode=x64
ChangesAssociations=yes
ChangesEnvironment=yes
Compression=lzma
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName} GDK
InfoBeforeFile=..\CHANGELOG
InfoAfterFile=..\README.md
LicenseFile=..\license.txt
SetupIconFile=..\msvs\minisphere.ico
SolidCompression=yes
UninstallDisplayName={#AppName} {#AppVersion}
UninstallDisplayIcon={app}\minisphere.exe,0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Components]
Name: "engine"; Description: "{#AppName} {#AppVersion} [Required]"; Types: full compact custom; Flags: fixed
Name: "spherun"; Description: "{#AppName} Console {#AppVersion} (spherun)"; Types: full; Flags: checkablealone
Name: "spherun/cell"; Description: "cell - Sphere packaging compiler"; Types: full
Name: "spherun/ssj"; Description: "ssj - JavaScript debugger"; Types: full

[Tasks]
Name: "assoc"; Description: "Associate these file extensions with minisphere:"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/sgm"; Description: ".sgm - Sphere game manifest (game.sgm)"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/s2gm"; Description: ".s2gm - Sphere 2.0 game manifest (game.s2gm)"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/spk"; Description: ".spk - Sphere SPK game package"; GroupDescription: "Automatically open Sphere file types:"
Name: "path"; Description: "Add {#AppName} Console (spherun) to the PATH"; GroupDescription: "Develop on the command line:"; Flags: checkedonce unchecked

[Files]
Source: "..\msw\minisphere.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: engine
Source: "..\msw\spherun.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun
Source: "..\msw\cell.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/cell
Source: "..\msw\ssj.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/ssj
Source: "..\msw64\minisphere.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: engine; Check: IsWin64
Source: "..\msw64\spherun.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun; Check: IsWin64
Source: "..\msw64\cell.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/cell; Check: IsWin64
Source: "..\msw64\ssj.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/ssj; Check: IsWin64
Source: "..\msw\startup.spk"; DestDir: "{app}"; Components: engine; Flags: ignoreversion
Source: "..\msw\documentation\minisphere-api.txt"; DestDir: "{app}\documentation"; Flags: ignoreversion; Components: spherun
Source: "..\msw\documentation\cell-api.txt"; DestDir: "{app}\documentation"; Flags: ignoreversion; Components: spherun/cell
Source: "..\msw\system\*"; DestDir: "{app}\system"; Components: engine; Flags: ignoreversion recursesubdirs createallsubdirs

[Registry]
; File Associations
Root: HKCR; Subkey: ".spk"; ValueType: string; ValueName: ""; ValueData: "minisphere.SPK"; Flags: uninsdeletevalue; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Package"; Flags: uninsdeletekey; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\minisphere.exe,0"; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\minisphere.exe"" ""%1"""; Tasks: assoc/spk
Root: HKCR; Subkey: ".sgm"; ValueType: string; ValueName: ""; ValueData: "minisphere.SGM"; Flags: uninsdeletevalue; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Manifest"; Flags: uninsdeletekey; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\minisphere.exe,0"; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\minisphere.exe"" ""%1"""; Tasks: assoc/sgm
Root: HKCR; Subkey: ".s2gm"; ValueType: string; ValueName: ""; ValueData: "minisphere.SGM"; Flags: uninsdeletevalue; Tasks: assoc/s2gm
Root: HKCR; Subkey: "minisphere.S2GM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\minisphere.exe,0"; Tasks: assoc/s2gm
Root: HKCR; Subkey: "minisphere.S2GM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\minisphere.exe"" ""%1"""; Tasks: assoc/s2gm
Root: HKCR; Subkey: "minisphere.S2GM"; ValueType: string; ValueName: ""; ValueData: "Sphere 2 Game Manifest"; Flags: uninsdeletekey; Tasks: assoc/s2gm
; PATH
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{app};{olddata}"; Tasks: path

[Icons]
Name: "{commonprograms}\{#AppName}"; Filename: "{app}\minisphere.exe"
Name: "{group}\API Reference - minisphere"; Filename: "{app}\documentation\minisphere-api.txt"; Components: spherun
Name: "{group}\API Reference - Cell"; Filename: "{app}\documentation\cell-api.txt"; Components: spherun/cell
