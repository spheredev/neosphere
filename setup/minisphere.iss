; minisphere installer for Windows releases
; (C) 2015-2016 Fat Cerberus
; Inno Setup 5.5 or later with ISPP is required.
;
; this will build a Windows installer for minisphere.  before compiling the installer,
; you must first build the complete minisphere GDK for both x86 and x64 using Visual Studio.
; in practical terms, this means building the full GDK and Redist solution configurations.

#define AppName "minisphere"
#define AppPublisher "Fat Cerberus"
#define AppVersion2 "3.1"
#define AppVersion3 "3.1a1"

; to create a bundle with Sphere Studio, copy the Sphere Studio binaries
; into msw/ide/ before building the installer.
#ifexist "../msw/ide/Sphere Studio.exe"
#define HAVE_SPHERE_STUDIO
#endif

[Setup]
OutputBaseFilename=minisphereSetup-{#AppVersion3}
OutputDir=.
AppId={{10C19C9F-1E29-45D8-A534-8FEF98C7C2FF}
AppName={#AppName}
AppVerName={#AppName} {#AppVersion3}
AppVersion={#AppVersion3}
AppPublisher=Fat Cerberus
AppUpdatesURL=http://forums.spheredev.org/index.php/topic,1215.0.html
AppCopyright=© 2015-2016 Fat Cerberus
AlwaysShowDirOnReadyPage=yes
ArchitecturesInstallIn64BitMode=x64
ChangesAssociations=yes
ChangesEnvironment=yes
Compression=lzma
DefaultDirName={pf}\{#AppName}
DefaultGroupName=Sphere 2.0 GDK
DisableProgramGroupPage=yes
DisableWelcomePage=no
InfoBeforeFile=changelog.rtf
InfoAfterFile=release.rtf
LicenseFile=../LICENSE.txt
SetupIconFile=..\msvs\spherical.ico
SolidCompression=yes
UninstallDisplayName={#AppName} {#AppVersion3}
UninstallDisplayIcon={app}\minisphere.exe,0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "normal"; Description: "Standard Redistributable (engine only)"
Name: "developer"; Description: "Sphere 2.0 Game Development Kit (GDK)"
Name: "custom"; Description: "Custom Installation"; Flags: iscustom

[Components]
Name: "engine"; Description: "{#AppName} {#AppVersion3} [Required]"; Types: normal developer custom; Flags: fixed
Name: "spherun"; Description: "Sphere 2.0 Game Development Kit"; Types: developer; Flags: checkablealone
#ifdef HAVE_SPHERE_STUDIO
Name: "spherun/ide"; Description: "Sphere Studio Integrated Development Environment"; Types: developer
#endif
Name: "spherun/cell"; Description: "cell - Sphere 2.0 Game Compiler"; Types: developer
Name: "spherun/ssj"; Description: "ssj - Sphere 2.0 JavaScript Debugger"; Types: developer

[Tasks]
Name: "assoc"; Description: "&Associate these file extensions with minisphere:"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/sgm"; Description: ".sgm - Sphere &game manifest (game.sgm)"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/s2gm"; Description: ".s2gm - Sphere 2.0 game &manifest (game.s2gm)"; GroupDescription: "Automatically open Sphere file types:"
Name: "assoc/spk"; Description: ".spk - Sphere &SPK game package"; GroupDescription: "Automatically open Sphere file types:"
#ifdef HAVE_SPHERE_STUDIO
Name: "assoc_ss"; Description: "&Associate these file extensions with Sphere Studio:"; GroupDescription: "Automatically open Sphere file types:"; Components: spherun/ide
Name: "assoc_ss/ssproj"; Description: ".ssproj - Sphere Studio project file"; GroupDescription: "Automatically open Sphere file types:"; Components: spherun/ide
#endif
Name: "path"; Description: "Add the GDK tools to the system %&PATH%"; GroupDescription: "Develop on the command line:"; Components: spherun; Flags: checkedonce unchecked

[Files]
Source: "..\msw\minisphere.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: engine
Source: "..\msw\spherun.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun
Source: "..\msw\cell.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/cell
Source: "..\msw\ssj.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/ssj
Source: "..\msw\gdk-cp.bat"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun
Source: "..\msw64\minisphere.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: engine; Check: IsWin64
Source: "..\msw64\spherun.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun; Check: IsWin64
Source: "..\msw64\cell.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/cell; Check: IsWin64
Source: "..\msw64\ssj.exe"; DestDir: "{app}"; Flags: ignoreversion; Components: spherun/ssj; Check: IsWin64
Source: "..\msw\documentation\sphere-api.txt"; DestDir: "{app}\documentation"; Flags: ignoreversion; Components: spherun
Source: "..\msw\documentation\cell-api.txt"; DestDir: "{app}\documentation"; Flags: ignoreversion; Components: spherun/cell
Source: "..\msw\documentation\miniRT-api.txt"; DestDir: "{app}\documentation"; Flags: ignoreversion; Components: spherun
Source: "..\msw\system\*"; DestDir: "{app}\system"; Components: engine; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\msw\template\*"; DestDir: "{app}\template"; Components: engine; Flags: ignoreversion recursesubdirs createallsubdirs
#ifdef HAVE_SPHERE_STUDIO
Source: "..\msw\ide\*"; DestDir: "{app}\ide"; Flags: ignoreversion recursesubdirs; Components: spherun/ide
#endif

[Registry]
; minisphere associations
Root: HKCR; Subkey: ".spk"; ValueType: string; ValueName: ""; ValueData: "minisphere.SPK"; Flags: uninsdeletevalue; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK"; ValueType: string; ValueName: ""; ValueData: "Sphere Game Package"; Flags: uninsdeletekey; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\minisphere.exe,0"; Tasks: assoc/spk
Root: HKCR; Subkey: "minisphere.SPK\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\minisphere.exe"" ""%1"""; Tasks: assoc/spk
Root: HKCR; Subkey: ".sgm"; ValueType: string; ValueName: ""; ValueData: "minisphere.SGM"; Flags: uninsdeletevalue; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM"; ValueType: string; ValueName: ""; ValueData: "Sphere 1.x Game Manifest"; Flags: uninsdeletekey; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\minisphere.exe,0"; Tasks: assoc/sgm
Root: HKCR; Subkey: "minisphere.SGM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\minisphere.exe"" ""%1"""; Tasks: assoc/sgm
Root: HKCR; Subkey: ".s2gm"; ValueType: string; ValueName: ""; ValueData: "minisphere.S2GM"; Flags: uninsdeletevalue; Tasks: assoc/s2gm
Root: HKCR; Subkey: "minisphere.S2GM"; ValueType: string; ValueName: ""; ValueData: "Sphere 2.0 Game Manifest"; Flags: uninsdeletekey; Tasks: assoc/s2gm
Root: HKCR; Subkey: "minisphere.S2GM\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\minisphere.exe,0"; Tasks: assoc/s2gm
Root: HKCR; Subkey: "minisphere.S2GM\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\minisphere.exe"" ""%1"""; Tasks: assoc/s2gm
; Sphere Studio associations
#ifdef HAVE_SPHERE_STUDIO
Root: HKCR; Subkey: ".ssproj"; ValueType: string; ValueName: ""; ValueData: "minisphere.SSPROJ"; Flags: uninsdeletevalue; Tasks: assoc_ss/ssproj
Root: HKCR; Subkey: "minisphere.SSPROJ"; ValueType: string; ValueName: ""; ValueData: "Sphere Studio Project"; Flags: uninsdeletekey; Tasks: assoc_ss/ssproj
Root: HKCR; Subkey: "minisphere.SSPROJ\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\ide\Sphere Studio.exe,0"; Tasks: assoc_ss/ssproj
Root: HKCR; Subkey: "minisphere.SSPROJ\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\ide\Sphere Studio.exe"" ""%1"""; Tasks: assoc_ss/ssproj
#endif
; PATH
Root: HKLM; Subkey: "SYSTEM\CurrentControlSet\Control\Session Manager\Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{app};{olddata}"; Tasks: path

[Icons]
Name: "{commonprograms}\{#AppName}"; Filename: "{app}\minisphere.exe"
#ifdef HAVE_SPHERE_STUDIO
Name: "{group}\Sphere Studio IDE"; Filename: "{app}\ide\Sphere Studio.exe"; Components: spherun/ide
#endif
Name: "{group}\Sphere 2.0 Command Prompt"; Filename: "%comspec%"; Parameters: "/k ""{app}\gdk-cp.bat"""; Components: spherun
Name: "{group}\Sphere 2.0 API Reference"; Filename: "{app}\documentation\sphere-api.txt"; Components: spherun
Name: "{group}\Cellscript API Reference"; Filename: "{app}\documentation\cell-api.txt"; Components: spherun/cell
Name: "{group}\miniRT API Reference"; Filename: "{app}\documentation\miniRT-api.txt"; Components: spherun

[Run]
#ifdef HAVE_SPHERE_STUDIO
Filename: "{app}\ide\Sphere Studio.exe"; Description: "Launch Sphere Studio now"; Flags: postinstall nowait skipifsilent
#endif

[Code]
procedure InitializeWizard;
begin
  WizardForm.LicenseAcceptedRadio.Checked := True;
end;
