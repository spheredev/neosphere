; minisphere installer for Windows releases
; (C) 2015 Fat Cerberus
; Inno Setup 5.5 or later with ISPP is required.
;
; This will build an installer for minisphere. Before compiling the
; installer, you must first build the engine in bin/ using Visual Studio
; or MSBuild.

; the installer can optionally be built with Sphere Studio included.
; to create such a package, you will need to place the Sphere Studio
; binaries in bin/sphereStudio before compiling the installer.
#ifexist "..\bin\sphereStudio\Sphere Studio.exe"
#define WANT_SPHERE_STUDIO
#define SphereStudioVersion "1.2.0"
#endif

; after copying the sphereStudio binaries into bin/, you can still build
; an installer without them by uncommenting the line below.
;#undef WANT_SPHERE_STUDIO

#define AppName "minisphere"
#define AppVersion "1.4.4"
#define AppFullVersion "1.4.4.701"
#define AppPublisher "Fat Cerberus"

#define AppExeName "engine.exe"
#define AppExeName_64 "engine64.exe"
#define AppExeName2 "console.exe"
#define AppExeName2_64 "console64.exe"

[Setup]
#ifdef WANT_SPHERE_STUDIO
AppId={{10C19C9F-1E29-45D8-A534-8FEF98C7C2FF}
#else
AppId={{79895E2D-DF9D-4534-9DB8-B3CD2AA92A7A}
#endif
AppName={#AppName}
AppVersion={#AppFullVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher=Fat Cerberus
AppUpdatesURL=http://forums.spheredev.org/index.php/topic,1215.0.html
DefaultDirName={pf}\{#AppName}
DefaultGroupName={#AppName}
SetupIconFile=..\msvc\minisphere.ico
InfoBeforeFile=D:\minisphere\bin\changelog.txt
#ifdef WANT_SPHERE_STUDIO
OutputBaseFilename=minisphere-{#AppVersion}-dev
#else
OutputBaseFilename=minisphere-{#AppVersion}-redist
#endif
OutputDir=.
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64
ChangesAssociations=yes
UninstallDisplayIcon={app}\engine.exe,0
#ifdef WANT_SPHERE_STUDIO
UninstallDisplayName={#AppName} {#AppVersion} with Sphere Studio
#else
UninstallDisplayName={#AppName} {#AppVersion}
#endif

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Components]
Name: "engine"; Description: "{#AppName} {#AppVersion}"; Types: full compact custom; Flags: fixed
Name: "console"; Description: "{#AppName} Console {#AppVersion}"; Types: full
#ifdef WANT_SPHERE_STUDIO
Name: "studio"; Description: "Sphere Studio {#SphereStudioVersion}"; Types: full; Check: DirExists('..\bin\sphereStudio')
#endif
Name: "docs"; Description: "{#AppName} Documentation"; Types: full compact
Name: "docs\api"; Description: "API Reference"; Types: full compact
Name: "games"; Description: "Sample Games"; Types: full

[Tasks]
Name: "assoc_spk"; Description: "Associate .spk (game packages) with {#AppName}"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc_sgm"; Description: "Associate .sgm (Sphere projects) with:"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc_sgm\engine"; Description: "{#AppName}"; Flags: exclusive
Name: "assoc_sgm\console"; Description: "{#AppName} Console"; Flags: exclusive; Components: console
#ifdef WANT_SPHERE_STUDIO
Name: "assoc_sgm\studio"; Description: "Sphere Studio"; Flags: exclusive; Components: studio
Name: "desktop"; Description: "Create a desktop icon for Sphere Studio"; GroupDescription: "{cm:AdditionalIcons}"; Components: studio
#endif

[Files]
Source: "..\bin\engine.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: engine
Source: "..\bin\engine64.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: engine; Check: IsWin64
Source: "..\bin\console.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: console
Source: "..\bin\console64.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: console; Check: IsWin64
Source: "..\bin\startup.spk"; DestDir: "{app}"; Flags: ignoreversion; Components: engine
Source: "..\bin\license.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: engine
Source: "..\bin\readme.md"; DestDir: "{app}"; Flags: ignoreversion; Components: engine
Source: "..\bin\changelog.txt"; DestDir: "{app}"; Flags: ignoreversion; Components: engine
Source: "..\bin\system\*"; DestDir: "{app}\system"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: engine
Source: "..\bin\documentation\*"; DestDir: "{app}\documentation"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: docs\api
Source: "..\bin\games\*"; DestDir: "{app}\games"; Flags: ignoreversion; Components: games
#ifdef WANT_SPHERE_STUDIO
Source: "..\bin\sphereStudio\*"; DestDir: "{app}\sphereStudio"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: studio
#endif

[INI]
Filename: "{userdocs}\Sphere Studio\Presets\minisphere.preset"; Section: "Sphere Studio"; Key: "sphere_path"; String: "{app}\{#AppExeName}"
Filename: "{userdocs}\Sphere Studio\Presets\minisphere.preset"; Section: "Sphere Studio"; Key: "sphere64_path"; String: "{app}\{#AppExeName_64}"; Check: IsWin64
Filename: "{userdocs}\Sphere Studio\Presets\minisphere.preset"; Section: "Sphere Studio"; Key: "plugins"; String: "FontEditPlugin,ImageEditPlugin,MapEditPlugin,ScriptEditPlugin,SoundTestPlugin,SPKPackerPlugin,SpritesetEditPlugin,TaskListPlugin,WindowstyleEditPlugin"; Flags: createkeyifdoesntexist
Filename: "{userdocs}\Sphere Studio\Presets\minisphere Console.preset"; Section: "Sphere Studio"; Key: "sphere_path"; String: "{app}\{#AppExeName2}"; Components: console
Filename: "{userdocs}\Sphere Studio\Presets\minisphere Console.preset"; Section: "Sphere Studio"; Key: "sphere64_path"; String: "{app}\{#AppExeName2_64}"; Components: console; Check: IsWin64
Filename: "{userdocs}\Sphere Studio\Presets\minisphere Console.preset"; Section: "Sphere Studio"; Key: "plugins"; String: "FontEditPlugin,ImageEditPlugin,MapEditPlugin,ScriptEditPlugin,SoundTestPlugin,SPKPackerPlugin,SpritesetEditPlugin,TaskListPlugin,WindowstyleEditPlugin"; Flags: createkeyifdoesntexist
Filename: "{userdocs}\Sphere Studio\Settings\editor.ini"; Section: "Sphere Studio"; Key: "last_preset"; String: "minisphere"
Filename: "{userdocs}\Sphere Studio\Settings\editor.ini"; Section: "Sphere Studio"; Key: "last_preset"; String: "minisphere Console"; Components: console

[Registry]
Root: HKCR; Subkey: ".spk"; ValueType: string; ValueName: ""; ValueData: "minisphere.SPK"; Flags: uninsdeletevalue; Tasks: assoc_spk
Root: HKCR; Subkey: "minisphere.SPK"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Package"; Flags: uninsdeletekey; Tasks: assoc_spk
Root: HKCR; Subkey: "minisphere.SPK\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"; Tasks: assoc_spk
Root: HKCR; Subkey: "minisphere.SPK\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: assoc_spk
Root: HKCR; Subkey: "minisphere.SPK\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName_64}"" ""%1"""; Tasks: assoc_spk; Check: IsWin64
Root: HKCR; Subkey: ".sgm"; ValueType: string; ValueName: ""; ValueData: "minisphere.SGM"; Flags: uninsdeletevalue; Tasks: assoc_sgm
Root: HKCR; Subkey: "minisphere.SGM"; ValueType: string; ValueName: ""; ValueData: "Sphere Studio Project"; Flags: uninsdeletekey; Tasks: assoc_sgm
Root: HKCR; Subkey: "minisphere.SGM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#AppExeName},0"; Tasks: assoc_sgm
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName2}"" ""%1"""; Tasks: assoc_sgm\console
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName2_64}"" ""%1"""; Tasks: assoc_sgm\console; Check: IsWin64
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName}"" ""%1"""; Tasks: assoc_sgm\engine
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExeName_64}"" ""%1"""; Tasks: assoc_sgm\engine; Check: IsWin64
#ifdef WANT_SPHERE_STUDIO
Root: HKCR; Subkey: "minisphere.SGM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\sphereStudio\Sphere Studio.exe"",0"; Tasks: assoc_sgm\studio
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\sphereStudio\Sphere Studio.exe"" ""%1"""; Tasks: assoc_sgm\studio
#endif

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName_64}"; Check: IsWin64
Name: "{group}\{#AppName} Console"; Filename: "{app}\{#AppExeName2}"
Name: "{group}\{#AppName} Console"; Filename: "{app}\{#AppExeName2_64}"; Check: IsWin64
Name: "{group}\Documentation\JS API Reference"; Filename: "{app}\documentation\minisphere-api.txt"; Flags: createonlyiffileexists
Name: "{group}\Documentation\License"; Filename: "{app}\license.txt"
#ifdef WANT_SPHERE_STUDIO
Name: "{commonprograms}\Sphere Studio"; Filename: "{app}\sphereStudio\Sphere Studio.exe"; Flags: createonlyiffileexists
Name: "{commondesktop}\Sphere Studio"; Filename: "{app}\sphereStudio\Sphere Studio.exe"; Tasks: desktop
#endif

[Run]
#ifdef WANT_SPHERE_STUDIO
Filename: "{app}\sphereStudio\Sphere Studio.exe"; Description: "{cm:LaunchProgram,Sphere Studio}"; Flags: nowait postinstall skipifsilent; Components: studio
#endif
