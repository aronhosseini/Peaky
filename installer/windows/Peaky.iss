; Peaky - Windows installer (Inno Setup 6.3+)  -  Developed by Aron H
; Build:  iscc installer\windows\Peaky.iss /DAppVersion=1.0.0 /DVstSource="C:\path\to\Peaky.vst3"
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef VstSource
  #define VstSource "..\..\build\Peaky_artefacts\Release\VST3\Peaky.vst3"
#endif

[Setup]
AppId={{6F1B3C52-9D4A-4B7E-A8C1-5E2D7A90B4F3}
AppName=Peaky
AppVersion={#AppVersion}
AppPublisher=Aron H
AppPublisherURL=https://www.instagram.com/aronh.official
AppSupportURL=https://www.instagram.com/aronh.official
DefaultDirName={autopf}\Peaky
DisableDirPage=yes
DefaultGroupName=Peaky
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
OutputDir=Output
OutputBaseFilename=Peaky-{#AppVersion}-Windows-VST3-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=Peaky (VST3)

[Files]
Source: "{#VstSource}\*"; DestDir: "{commoncf64}\VST3\Peaky.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\..\Presets\Factory\*.peakypreset"; DestDir: "{userdocs}\Aron H\Peaky\Factory Preset Files"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Peaky on Instagram (Aron H)"; Filename: "https://www.instagram.com/aronh.official"

[Run]
Filename: "https://www.instagram.com/aronh.official"; Description: "Follow Aron H on Instagram"; Flags: postinstall shellexec skipifsilent unchecked
