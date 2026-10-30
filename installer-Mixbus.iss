[Setup]
AppName=Mixbus Chain
AppVersion=0.1.0
AppPublisher=DaniArieta
DefaultDirName={autopf}\Mixbus Chain
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=output
OutputBaseFilename=MixbusChain-Setup
Compression=lzma
SolidCompression=yes

[Files]
Source: "payload\Mixbus Chain.vst3\*"; DestDir: "{commonpf64}\Common Files\VST3\Mixbus Chain.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
