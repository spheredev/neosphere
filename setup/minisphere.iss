; minisphere installer for Windows releases
; (C) 2015 Fat Cerberus
; Inno Setup 5.5 or later with ISPP is required.
;
; This will build a Windows installer for minisphere. Before compiling the
; installer, you must first build the engine using Visual Studio or MSBuild.
; For a GDK installer, you will also need to build the Console engine and
; place the binaries for Sphere Studio in bin/spherestudio.
;
; To build the engine, enter the following commands from the directory where
; you checked out the source:
;     MSBuild msvc/minisphere.sln /p:Configuration=Release /p:Platform=Win32
;     MSBuild msvc/minisphere.sln /p:Configuration=Release /p:Platform=x64
;
; And for minisphere Console:
;     MSBuild msvc/minisphere.sln /p:Configuration=Console /p:Platform=Win32
;     MSBuild msvc/minisphere.sln /p:Configuration=Console /p:Platform=x64


; if, after copying the Sphere Studio binaries into bin/spherestudio, you
; still need to build an engine-only installer, you can do so by commenting out the
; line below:
#define WANT_GDK_SETUP

#define AppName "minisphere"
#define AppVersion "2.1a"
#define AppRawVersion "0.0.0.0"
#define AppPublisher "Fat Cerberus"

#ifdef WANT_GDK_SETUP
#define AppExeName "msphere-nc.exe"
#define AppExeName2 "msphere.exe"
#else
#define AppExeName "minisphere.exe"
#endif

[Setup]
#ifdef WANT_GDK_SETUP
AppId={{10C19C9F-1E29-45D8-A534-8FEF98C7C2FF}
OutputBaseFilename=minisphereGDK-{#AppVersion}
SetupIconFile=..\bin\spheredev.ico
InfoBeforeFile=..\changelog.txt
InfoAfterFile=..\readme.md
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
SetupIconFile=..\bin\spherical.ico
AppName={#AppName}
AppVerName={#AppName} {#AppVersion}
DefaultDirName={pf}\{#AppName}
DisableProgramGroupPage=yes
UninstallDisplayName={#AppName} {#AppVersion}
UninstallDisplayIcon={app}\spherical.ico,0
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
#ifdef WANT_GDK_SETUP
Name: "engine"; Description: "{#AppName} Console {#AppVersion} (required)"; Types: full compact custom; Flags: fixed
Name: "cell"; Description: "Cell {#AppVersion}"; Types: full
Name: "studio"; Description: "Sphere Studio 1.2.0"; Types: full
Name: "docs"; Description: "{#AppName} Documentation"
Name: "docs/api"; Description: "API Reference"; Types: full compact
#endif

[Tasks]
Name: "assoc"; Description: "Associate these file extensions with {#AppName}:"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/sgm"; Description: ".sgm - Sphere game manifest (game.sgm)"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/s2gm"; Description: ".s2gm - Sphere 2.0 game manifest (game.s2gm)"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/spk"; Description: ".spk - Sphere SPK game package"; GroupDescription: "Automatically open Sphere file types:"
#ifdef WANT_GDK_SETUP
Name: "path"; Description: "Add {#AppName} GDK to the PATH"; GroupDescription: "Run minisphere tools from the command line:"; Flags: checkedonce unchecked
#endif

[Files]
#ifdef WANT_GDK_SETUP
Source: "..\bin\msphere.exe"; DestName: "{#AppExeName2}"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: engine
Source: "..\bin\msphere-nc.exe"; DestName: "{#AppExeName}"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: engine
Source: "..\bin\msphere64.exe"; DestName: "{#AppExeName2}"; DestDir: "{app}\bin"; Flags: ignoreversion; Check: IsWin64; Components: engine
Source: "..\bin\msphere64-nc.exe"; DestName: "{#AppExeName}"; DestDir: "{app}\bin"; Flags: ignoreversion; Check: IsWin64; Components: engine
Source: "..\bin\cell.exe"; DestDir: "{app}\bin"; Flags: ignoreversion; Components: cell
Source: "..\bin\spheredev.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\spherical.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\plugin\minisphereGdkPlugin.dll"; DestDir: "{commonappdata}\Sphere Studio\Plugins"; Flags: ignoreversion; Components: studio
Source: "..\bin\documentation\*"; DestDir: "{app}\documentation"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: docs/api
Source: "..\bin\system\*"; DestDir: "{app}\bin\system"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: engine
Source: "..\assets\template\*"; DestDir: "{app}\assets\template"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: studio
Source: "..\studio\*"; DestDir: "{app}\studio"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: studio
#else
Source: "..\bin\msphere-nc.exe"; DestName: "{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion; Check: not IsWin64
Source: "..\bin\msphere64-nc.exe"; DestName: "{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion; Check: IsWin64
Source: "..\bin\spherical.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\startup.spk"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\system\*"; DestDir: "{app}\system"; Flags: ignoreversion recursesubdirs createallsubdirs
#endif

[Registry]
Root: HKCR; Subkey: ".spk"; ValueType: string; ValueName: ""; ValueData: "minisphere.SPK"; Flags: uninsdeletevalue; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Package"; Flags: uninsdeletekey; Tasks: assoc/spk
Root: HKCR; Subkey: ".sgm"; ValueType: string; ValueName: ""; ValueData: "minisphere.SGM"; Flags: uninsdeletevalue; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Manifest"; Flags: uninsdeletekey; Tasks: assoc/sgm
Root: HKCR; Subkey: ".s2gm"; ValueType: string; ValueName: ""; ValueData: "minisphere.SGM"; Flags: uninsdeletevalue; Tasks: assoc/s2gm
Root: HKCR; Subkey: "minisphere.S2GM"; ValueType: string; ValueName: ""; ValueData: "Sphere 2 Game Manifest"; Flags: uninsdeletekey; Tasks: assoc/s2gm
#ifdef WANT_GDK_SETUP
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{app}\bin;{olddata}"; Tasks: path
Root: HKCR; Subkey: "minisphere.SPK\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\{#AppExeName},0"; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\{#AppExeName}"" ""%1"""; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SGM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\{#AppExeName},0"; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\{#AppExeName}"" ""%1"""; Tasks: assoc/sgm
Root: HKCR; Subkey: ".ssproj"; ValueType: string; ValueName: ""; ValueData: "minisphere.SSPROJ"; Flags: uninsdeletevalue; Components: studio
Root: HKCR; Subkey: "minisphere.SSPROJ"; ValueType: string; ValueName: ""; ValueData: "Sphere Studio Project"; Flags: uninsdeletekey; Components: studio
Root: HKCR; Subkey: "minisphere.SSPROJ\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\studio\Sphere Studio.exe,0"; Components: studio
Root: HKCR; Subkey: "minisphere.SSPROJ\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\studio\Sphere Studio.exe"" ""%1"""; Components: studio
#else
Root: HKCR; Subkey: "minisphere.SPK\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SGM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.S2GM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"; Tasks: assoc/s2gm
Root: HKCR; Subkey: "minisphere.S2GM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: assoc/s2gm
#endif

[Icons]
#ifdef WANT_GDK_SETUP
Name: "{group}\API Reference - Cell"; Filename: "{app}\documentation\cell-api.txt"; Components: docs/api
Name: "{group}\API Reference - minisphere"; Filename: "{app}\documentation\minisphere-api.txt"; Components: docs/api
Name: "{group}\{#AppName} Console"; Filename: "{app}\bin\{#AppExeName2}"; Components: engine
Name: "{group}\Sphere Studio"; Filename: "{app}\studio\Sphere Studio.exe"; Components: studio
#else
Name: "{commonprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"
#endif

[Run]
#ifdef WANT_GDK_SETUP
Filename: "{app}\studio\Sphere Studio.exe"; Description: "Launch Sphere Studio 1.2.0"; Flags: postinstall skipifsilent; Components: studio
#endif
