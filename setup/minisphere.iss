; minisphere installer for Windows releases
; (C) 2015 Fat Cerberus
; Inno Setup 5.5 or later with ISPP is required.
;
; This will build a Windows installer for minisphere. Before compiling the
; installer, you must first build the engine using Visual Studio or MSBuild.
; For a GDK installer, you will also need to build the Console engine and
; GDK tools (Cell and SSJ).
;
; To build the redistributable engine, enter the following commands from the
; directory where you checked out the source:
;     MSBuild msvc/minisphere.sln /p:Configuration=Release
;
; And for minisphere GDK:
;     MSBuild msvc/minisphere.sln /p:Configuration=Console


; if, after copying the Sphere Studio binaries into bin/spherestudio, you
; still need to build an engine-only installer, you can do so by commenting out the
; line below:
#define WANT_GDK_SETUP

#define AppName "minisphere"
#define AppVersion "3.0a0"
#define AppRawVersion "2.99.812"
#define AppPublisher "Fat Cerberus"

[Setup]
#ifdef WANT_GDK_SETUP
AppId={{10C19C9F-1E29-45D8-A534-8FEF98C7C2FF}
OutputBaseFilename=minisphereGDK-{#AppVersion}
SetupIconFile=spheredev.ico
InfoBeforeFile=..\CHANGELOG
InfoAfterFile=..\README.md
AppName={#AppName} GDK
AppVerName={#AppName} GDK {#AppVersion}
DefaultDirName={pf}\{#AppName} GDK
DefaultGroupName={#AppName} GDK
ChangesEnvironment=yes
UninstallDisplayName={#AppName} GDK {#AppVersion}
UninstallDisplayIcon={app}\spheredev.ico,0
#else
AppId={{79895E2D-DF9D-4534-9DB8-B3CD2AA92A7A}
OutputBaseFilename=minisphere-{#AppVersion}
SetupIconFile=spherical.ico
AppName={#AppName}
AppVerName={#AppName} {#AppVersion}
DefaultDirName={pf}\{#AppName}
DisableProgramGroupPage=yes
UninstallDisplayName={#AppName} {#AppVersion}
UninstallDisplayIcon={app}\spherun.exe,0
#endif
AppVersion={#AppRawVersion}
AppPublisher=Fat Cerberus
AppUpdatesURL=http://forums.spheredev.org/index.php/topic,1215.0.html
LicenseFile=..\license.txt
OutputDir=.
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
DisableDirPage=auto
AlwaysShowDirOnReadyPage=yes
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Components]
Name: "spherun"; Description: "{#AppName} {#AppVersion} (spherun) [Required]"; Types: full compact custom; Flags: fixed
#ifdef WANT_GDK_SETUP
Name: "console"; Description: "{#AppName} Console {#AppVersion}"; Types: full; Flags: checkablealone
Name: "console/cell"; Description: "cell - Sphere packaging compiler"; Types: full
Name: "console/ssj"; Description: "ssj - JavaScript debugger"; Types: full
#endif

[Tasks]
Name: "assoc"; Description: "Associate these file extensions with minisphere (spherun):"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/sgm"; Description: ".sgm - Sphere game manifest (game.sgm)"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/s2gm"; Description: ".s2gm - Sphere 2.0 game manifest (game.s2gm)"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/spk"; Description: ".spk - Sphere SPK game package"; GroupDescription: "Automatically open Sphere file types:"
#ifdef WANT_GDK_SETUP
Name: "path"; Description: "Add {#AppName} Console to the PATH"; GroupDescription: "Develop on the command line:"; Flags: checkedonce unchecked
#endif

[Files]
#ifdef WANT_GDK_SETUP
Source: "..\msw\sphere.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: console
Source: "..\msw\spherun.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun
Source: "..\msw\cell.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: console/cell
Source: "..\msw\ssj.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: console/ssj
Source: "..\msw64\sphere.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: console; Check: IsWin64
Source: "..\msw64\spherun.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun; Check: IsWin64
Source: "..\msw64\cell.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: console/cell; Check: IsWin64
Source: "..\msw64\ssj.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: console/ssj; Check: IsWin64
Source: "..\msw\startup.spk"; DestDir: "{app}"; Components: spherun; Flags: ignoreversion
Source: "..\msw\documentation\minisphere-api.txt"; DestDir: "{app}\documentation"; Flags: ignoreversion; Components: console
Source: "..\msw\documentation\cell-api.txt"; DestDir: "{app}\documentation"; Flags: ignoreversion; Components: console/cell
Source: "..\msw\system\*"; DestDir: "{app}\system"; Components: spherun; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "spheredev.ico"; DestDir: "{app}"; Flags: ignoreversion
#else
Source: "..\msw\spherun.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun
Source: "..\msw64\spherun.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun; Check: IsWin64
Source: "..\msw\startup.spk"; DestDir: "{app}"; Components: spherun; Flags: ignoreversion
Source: "..\msw\system\*"; DestDir: "{app}\system"; Components: spherun; Flags: ignoreversion recursesubdirs createallsubdirs
#endif

[Registry]
Root: HKCR; Subkey: ".spk"; ValueType: string; ValueName: ""; ValueData: "minisphere.SPK"; Flags: uninsdeletevalue; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Package"; Flags: uninsdeletekey; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\spherun.exe,0"; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\spherun.exe"" ""%1"""; Tasks: assoc/spk
Root: HKCR; Subkey: ".sgm"; ValueType: string; ValueName: ""; ValueData: "minisphere.SGM"; Flags: uninsdeletevalue; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Manifest"; Flags: uninsdeletekey; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\spherun.exe,0"; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\spherun.exe"" ""%1"""; Tasks: assoc/sgm
Root: HKCR; Subkey: ".s2gm"; ValueType: string; ValueName: ""; ValueData: "minisphere.SGM"; Flags: uninsdeletevalue; Tasks: assoc/s2gm
Root: HKCR; Subkey: "minisphere.S2GM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\spherun.exe,0"; Tasks: assoc/s2gm
Root: HKCR; Subkey: "minisphere.S2GM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\spherun.exe"" ""%1"""; Tasks: assoc/s2gm
Root: HKCR; Subkey: "minisphere.S2GM"; ValueType: string; ValueName: ""; ValueData: "Sphere 2 Game Manifest"; Flags: uninsdeletekey; Tasks: assoc/s2gm
#ifdef WANT_GDK_SETUP
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{app};{olddata}"; Tasks: path
#endif

[Icons]
Name: "{commonprograms}\{#AppName}"; Filename: "{app}\spherun.exe"
#ifdef WANT_GDK_SETUP
Name: "{group}\API Reference - minisphere"; Filename: "{app}\documentation\minisphere-api.txt"; Components: console
Name: "{group}\API Reference - Cell"; Filename: "{app}\documentation\cell-api.txt"; Components: console/cell
#endif
