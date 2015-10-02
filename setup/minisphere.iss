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
#define AppExeName "msphere.exe"
#define AppVersion "2.0b1"
#define AppRawVersion "1.99.99.1"
#define AppPublisher "Fat Cerberus"

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
DisableProgramGroupPage=yes
DisableDirPage=auto
AlwaysShowDirOnReadyPage=yes
ChangesAssociations=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Components]
#ifdef WANT_GDK_SETUP
Name: "minisphere"; Description: "{#AppName} Console {#AppVersion} (recommended)"; Types: full
Name: "cell"; Description: "Cell {#AppVersion}"; Types: full
Name: "docs"; Description: "{#AppName} Documentation"; Types: full compact
Name: "docs\api"; Description: "API Reference"; Types: full compact
#endif

[Tasks]
Name: "assoc"; Description: "Associate these file extensions with {#AppName}:"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/sgm"; Description: ".sgm - Sphere game manifest (game.sgm)"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/spk"; Description: ".spk - Sphere SPK game package"; GroupDescription: "Automatically open Sphere file types:"
Name: "path"; Description: "Add minisphere GDK to the PATH"; GroupDescription: "Run minisphere tools from the command line:"; Flags: unchecked

[Files]
#ifdef WANT_GDK_SETUP
Source: "..\bin\msphere.exe"; DestName: "{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion; Check: not IsWin64
Source: "..\bin\msphere64.exe"; DestName: "{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion; Check: IsWin64
Source: "..\bin\cell.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\spheredev.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\documentation\*"; DestDir: "{app}\documentation"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: docs\api
#else
Source: "..\bin\engine.exe"; DestName: "{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion; Check: not IsWin64
Source: "..\bin\engine64.exe"; DestName: "{#AppExeName}"; DestDir: "{app}"; Flags: ignoreversion; Check: IsWin64
#endif
Source: "..\bin\system\*"; DestDir: "{app}\system"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\bin\spherical.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\startup.spk"; DestDir: "{app}"; Flags: ignoreversion

[Registry]
Root: HKCR; Subkey: ".spk"; ValueType: string; ValueName: ""; ValueData: "minisphere.SPK"; Flags: uninsdeletevalue; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Package"; Flags: uninsdeletekey; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: assoc/spk
Root: HKCR; Subkey: ".sgm"; ValueType: string; ValueName: ""; ValueData: "minisphere.SGM"; Flags: uninsdeletevalue; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Manifest"; Flags: uninsdeletekey; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: assoc/sgm
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{app};{olddata}"

[Icons]
#ifndef WANT_GDK_SETUP
Name: "{commonprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"
#else
Name: "{commonprograms}\{#AppName} Console"; Filename: "{app}\{#AppExeName}"
#endif
