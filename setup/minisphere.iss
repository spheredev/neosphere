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
;#define WANT_GDK_SETUP

#ifnexist "..\bin\spherestudio\Sphere Studio.exe"
#undef WANT_GDK_SETUP
#endif

#define AppName "minisphere"
#define AppVersion "1.7.7"
#define AppBuildNumber "825"
#define AppPublisher "Fat Cerberus"

#define AppExeName "engine.exe"
#define AppExeName_64 "engine64.exe"
#define AppExeName2 "console.exe"
#define AppExeName2_64 "console64.exe"

#ifdef WANT_GDK_SETUP
#define SphereStudioVersion "1.2.0"
#endif

[Setup]
#ifdef WANT_GDK_SETUP
AppId={{10C19C9F-1E29-45D8-A534-8FEF98C7C2FF}
OutputBaseFilename=minisphereGDK-{#AppVersion}
SetupIconFile=..\bin\spherestudio.ico
InfoBeforeFile=..\changelog.txt
InfoAfterFile=..\readme.md
AppName={#AppName} GDK
AppVerName={#AppName} GDK {#AppVersion}
DefaultDirName={pf}\{#AppName} GDK
UninstallDisplayName={#AppName} GDK {#AppVersion}
UninstallDisplayIcon={app}\spherestudio.ico,0
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
AppVersion={#AppVersion}.{#AppBuildNumber}
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
Name: "engine"; Description: "{#AppName} {#AppVersion} (required)"; Types: full compact custom; Flags: fixed
Name: "console"; Description: "{#AppName} Console {#AppVersion} (recommended)"; Types: full
Name: "studio"; Description: "Sphere Studio {#SphereStudioVersion}"; Types: full
Name: "docs"; Description: "{#AppName} Documentation"; Types: full compact
Name: "docs\api"; Description: "API Reference"; Types: full compact
#endif

[Tasks]
Name: "assoc_spk"; Description: "Associate .spk (game package) with {#AppName}"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc_sgm"; Description: "Associate .sgm (unpackaged game) with:"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc_sgm\engine"; Description: "{#AppName}"; Flags: exclusive
#ifdef WANT_GDK_SETUP
Name: "assoc_sgm\console"; Description: "{#AppName} Console"; Flags: exclusive; Components: console
Name: "assoc_sgm\studio"; Description: "Sphere Studio"; Flags: exclusive unchecked; Components: studio
Name: "desktop"; Description: "Create a desktop icon for Sphere Studio"; GroupDescription: "{cm:AdditionalIcons}"; Components: studio
#endif

[Files]
; don't mind the preprocessor mess here, these are arranged to maximize compression.
; reordering them will likely make the installer larger.
#ifdef WANT_GDK_SETUP
Source: "..\bin\engine.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\console.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: console
Source: "..\bin\engine64.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: IsWin64
Source: "..\bin\console64.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: console; Check: IsWin64
#else
Source: "..\bin\engine.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\engine64.exe"; DestDir: "{app}"; Flags: ignoreversion; Check: IsWin64
Source: "..\bin\spherical.ico"; DestDir: "{app}"; Flags: ignoreversion
#endif
Source: "..\bin\system\*"; DestDir: "{app}\system"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\bin\spherical.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\startup.spk"; DestDir: "{app}"; Flags: ignoreversion
#ifdef WANT_GDK_SETUP
Source: "..\bin\spherestudio.ico"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bin\documentation\*"; DestDir: "{app}\documentation"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: docs\api
Source: "..\bin\spherestudio\*"; DestDir: "{app}\spherestudio"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: studio
#endif

[INI]
#ifdef WANT_GDK_SETUP
Filename: "{userdocs}\Sphere Studio\Presets\minisphere.preset"; Section: "Preset"; Key: "enginePath"; String: "{app}\{#AppExeName}"; Components: studio
Filename: "{userdocs}\Sphere Studio\Presets\minisphere.preset"; Section: "Preset"; Key: "enginePath64"; String: "{app}\{#AppExeName_64}"; Components: studio; Check: IsWin64
Filename: "{userdocs}\Sphere Studio\Presets\minisphere.preset"; Section: "Preset"; Key: "plugins"; String: "FontEditPlugin|ImageEditPlugin|MapEditPlugin|minisphereRemotePlugin|ScriptEditPlugin|SoundTestPlugin|SPKPackerPlugin|SpritesetEditPlugin|TaskListPlugin|WindowstyleEditPlugin"; Components: studio
Filename: "{userdocs}\Sphere Studio\Presets\minisphere.preset"; Section: "Preset"; Key: "defaultEditor"; String: "Script Editor"; Components: studio
Filename: "{userdocs}\Sphere Studio\Presets\minisphere Console.preset"; Section: "Preset"; Key: "enginePath"; String: "{app}\{#AppExeName2}"; Components: studio and console
Filename: "{userdocs}\Sphere Studio\Presets\minisphere Console.preset"; Section: "Preset"; Key: "enginePath64"; String: "{app}\{#AppExeName2_64}"; Components: studio and console; Check: IsWin64
Filename: "{userdocs}\Sphere Studio\Presets\minisphere Console.preset"; Section: "Preset"; Key: "plugins"; String: "FontEditPlugin|ImageEditPlugin|MapEditPlugin|minisphereRemotePlugin|ScriptEditPlugin|SoundTestPlugin|SPKPackerPlugin|SpritesetEditPlugin|TaskListPlugin|WindowstyleEditPlugin"; Components: studio and console
Filename: "{userdocs}\Sphere Studio\Presets\minisphere Console.preset"; Section: "Preset"; Key: "defaultEditor"; String: "Script Editor"; Components: studio and console
Filename: "{userdocs}\Sphere Studio\Settings\Sphere Studio.ini"; Section: "Sphere Studio"; Key: "setupComplete"; String: "True"; Components: studio
Filename: "{userdocs}\Sphere Studio\Settings\Sphere Studio.ini"; Section: "Sphere Studio"; Key: "preset"; String: "minisphere"; Components: studio
Filename: "{userdocs}\Sphere Studio\Settings\Sphere Studio.ini"; Section: "Sphere Studio"; Key: "preset"; String: "minisphere Console"; Components: studio and console
#endif

[Registry]
Root: HKCR; Subkey: ".spk"; ValueType: string; ValueName: ""; ValueData: "minisphere.SPK"; Flags: uninsdeletevalue; Tasks: assoc_spk
Root: HKCR; Subkey: "minisphere.SPK"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Package"; Flags: uninsdeletekey; Tasks: assoc_spk
Root: HKCR; Subkey: "minisphere.SPK\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"; Tasks: assoc_spk
Root: HKCR; Subkey: "minisphere.SPK\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: assoc_spk
Root: HKCR; Subkey: "minisphere.SPK\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName_64}"" ""%1"""; Tasks: assoc_spk; Check: IsWin64
Root: HKCR; Subkey: ".sgm"; ValueType: string; ValueName: ""; ValueData: "minisphere.SGM"; Flags: uninsdeletevalue; Tasks: assoc_sgm
Root: HKCR; Subkey: "minisphere.SGM"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Definition"; Flags: uninsdeletekey; Tasks: assoc_sgm
Root: HKCR; Subkey: "minisphere.SGM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"; Tasks: assoc_sgm
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: assoc_sgm\engine
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName_64}"" ""%1"""; Tasks: assoc_sgm\engine; Check: IsWin64
#ifdef WANT_GDK_SETUP
Root: HKCR; Subkey: "minisphere.SGM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\spherestudio\Sphere Studio.exe"",0"; Components: studio; Tasks: assoc_sgm\studio
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName2}"" ""%1"""; Tasks: assoc_sgm\console
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName2_64}"" ""%1"""; Tasks: assoc_sgm\console; Check: IsWin64
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\spherestudio\Sphere Studio.exe"" ""%1"""; Tasks: assoc_sgm\studio
Root: HKCR; Subkey: ".ssproj"; ValueType: string; ValueName: ""; ValueData: "minisphere.ssproj"; Flags: uninsdeletevalue; Components: studio
Root: HKCR; Subkey: "minisphere.ssproj"; ValueType: string; ValueName: ""; ValueData: "Sphere Studio Project"; Flags: uninsdeletekey; Components: studio
Root: HKCR; Subkey: "minisphere.ssproj\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\spherestudio\Sphere Studio.exe"",0"; Components: studio
Root: HKCR; Subkey: "minisphere.ssproj\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\spherestudio\Sphere Studio.exe"" ""%1"""; Components: studio
#endif

[Icons]
Name: "{commonprograms}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{commonprograms}\{#AppName}"; Filename: "{app}\{#AppExeName_64}"; Check: IsWin64
#ifdef WANT_GDK_SETUP
Name: "{commonprograms}\Sphere Studio"; Filename: "{app}\spherestudio\Sphere Studio.exe"; Components: studio
Name: "{commondesktop}\Sphere Studio"; Filename: "{app}\spherestudio\Sphere Studio.exe"; Components: studio; Tasks: desktop
Name: "{commonprograms}\{#AppName} Console"; Filename: "{app}\{#AppExeName2}"; Components: console
Name: "{commonprograms}\{#AppName} Console"; Filename: "{app}\{#AppExeName2_64}"; Components: console; Check: IsWin64
#endif

[Run]
#ifdef WANT_GDK_SETUP
Filename: "{app}\spherestudio\Sphere Studio.exe"; Description: "{cm:LaunchProgram,Sphere Studio}"; Flags: nowait postinstall skipifsilent; Components: studio
#endif
