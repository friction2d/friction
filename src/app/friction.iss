#define MyAppName "Friction"
#define MyAppVersion "@PROJECT_VERSION@"
#define MyAppPublisher "Friction"
#define MyAppURL "https://friction.graphics"
#define MyAppExeName "friction.exe"
#define MyAppAssocName MyAppName + " Project"
#define MyAppAssocExt ".friction"
#define MyAppAssocKey StringChange(MyAppAssocName, " ", "") + MyAppAssocExt

[Setup]
AppId={{12D25577-88FD-4C5A-AF69-CCA597CAC44E}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={commonpf64}\{#MyAppName}
ChangesAssociations=yes
DisableProgramGroupPage=yes
LicenseFile=LICENSE.md
PrivilegesRequiredOverridesAllowed=dialog
OutputDir=setup
OutputBaseFilename=friction
SetupIconFile=friction.ico
Compression=lzma
SolidCompression=yes
WizardStyle=modern
WizardSmallImageFile=friction.bmp
WizardImageStretch=yes

[Components]
Name: "friction"; Description: "Friction"; Types: full compact custom; Flags: fixed
Name: "ffmpeg"; Description: "FFmpeg"; Types: full compact custom; Flags: fixed
Name: "qt"; Description: "Qt"; Types: full compact custom; Flags: fixed
Name: "svgo"; Description: "SVG Optimizer"; Types: full

[Files]
Source: "{#MyAppExeName}"; DestDir: "{app}"; Components: friction ; Flags: ignoreversion
Source: "frictioncore.dll"; DestDir: "{app}"; Components: friction ; Flags: ignoreversion
Source: "frictionui.dll"; DestDir: "{app}"; Components: friction ; Flags: ignoreversion
Source: "skia.dll"; DestDir: "{app}"; Components: friction ; Flags: ignoreversion

Source: "svgo-win.exe"; DestDir: "{app}"; Components: svgo ; Flags: ignoreversion
Source: "svgo-license.txt"; DestDir: "{app}"; Components: svgo ; Flags: ignoreversion

Source: "avcodec-58.dll"; DestDir: "{app}"; Components: ffmpeg ; Flags: ignoreversion
Source: "avdevice-58.dll"; DestDir: "{app}"; Components: ffmpeg ; Flags: ignoreversion
Source: "avformat-58.dll"; DestDir: "{app}"; Components: ffmpeg ; Flags: ignoreversion
Source: "avutil-56.dll"; DestDir: "{app}"; Components: ffmpeg ; Flags: ignoreversion
Source: "swresample-3.dll"; DestDir: "{app}"; Components: ffmpeg ; Flags: ignoreversion
Source: "swscale-5.dll"; DestDir: "{app}"; Components: ffmpeg ; Flags: ignoreversion

Source: "qscintilla2_qt6.dll"; DestDir: "{app}"; Components: qt ; Flags: ignoreversion
Source: "Qt6Core.dll"; DestDir: "{app}"; Components: qt ; Flags: ignoreversion
Source: "Qt6Gui.dll"; DestDir: "{app}"; Components: qt ; Flags: ignoreversion
Source: "Qt6Multimedia.dll"; DestDir: "{app}"; Components: qt ; Flags: ignoreversion
Source: "Qt6Network.dll"; DestDir: "{app}"; Components: qt ; Flags: ignoreversion
Source: "Qt6OpenGL.dll"; DestDir: "{app}"; Components: qt ; Flags: ignoreversion
Source: "Qt6Qml.dll"; DestDir: "{app}"; Components: qt ; Flags: ignoreversion
Source: "Qt6Widgets.dll"; DestDir: "{app}"; Components: qt ; Flags: ignoreversion
Source: "Qt6Xml.dll"; DestDir: "{app}"; Components: qt ; Flags: ignoreversion
Source: "audio\*"; DestDir: "{app}\audio"; Components: qt ; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "platforms\*"; DestDir: "{app}\platforms"; Components: qt ; Flags: ignoreversion recursesubdirs createallsubdirs

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Registry]
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocExt}\OpenWithProgids"; ValueType: string; ValueName: "{#MyAppAssocKey}"; ValueData: ""; Flags: uninsdeletevalue
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}"; ValueType: string; ValueName: ""; ValueData: "{#MyAppAssocName}"; Flags: uninsdeletekey
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\{#MyAppExeName},0"
Root: HKA; Subkey: "Software\Classes\{#MyAppAssocKey}\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#MyAppExeName}"" ""%1"""
Root: HKA; Subkey: "Software\Classes\Applications\{#MyAppExeName}\SupportedTypes"; ValueType: string; ValueName: ".friction"; ValueData: ""

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

; Track all files installed since first release (never break upgrades!)
[InstallDelete]
Type: files; Name: {app}\friction.exe
Type: files; Name: {app}\frictioncore.dll
Type: files; Name: {app}\frictionui.dll
Type: files; Name: {app}\avcodec-58.dll
Type: files; Name: {app}\avdevice-58.dll
Type: files; Name: {app}\avformat-58.dll
Type: files; Name: {app}\avutil-56.dll
Type: files; Name: {app}\qscintilla2_qt6.dll
Type: files; Name: {app}\Qt6Concurrent.dll
Type: files; Name: {app}\Qt6Core.dll
Type: files; Name: {app}\Qt6Gui.dll
Type: files; Name: {app}\Qt6Multimedia.dll
Type: files; Name: {app}\Qt6Network.dll
Type: files; Name: {app}\Qt6OpenGL.dll
Type: files; Name: {app}\Qt6PrintSupport.dll
Type: files; Name: {app}\Qt6Qml.dll
Type: files; Name: {app}\Qt6Svg.dll
Type: files; Name: {app}\Qt6Widgets.dll
Type: files; Name: {app}\Qt6Xml.dll
Type: files; Name: {app}\swresample-3.dll
Type: files; Name: {app}\swscale-5.dll
Type: files; Name: {app}\audio\qtaudio_wasapi.dll
Type: files; Name: {app}\audio\qtaudio_windows.dll
Type: files; Name: {app}\imageformats\qgif.dll
Type: files; Name: {app}\imageformats\qicns.dll
Type: files; Name: {app}\imageformats\qico.dll
Type: files; Name: {app}\imageformats\qjpeg.dll
Type: files; Name: {app}\imageformats\qsvg.dll
Type: files; Name: {app}\imageformats\qtga.dll
Type: files; Name: {app}\imageformats\qtiff.dll
Type: files; Name: {app}\imageformats\qwbmp.dll
Type: files; Name: {app}\imageformats\qwebp.dll
Type: files; Name: {app}\platforms\qdirect2d.dll
Type: files; Name: {app}\platforms\qminimal.dll
Type: files; Name: {app}\platforms\qoffscreen.dll
Type: files; Name: {app}\platforms\qwindows.dll
Type: files; Name: {app}\presets\expressions\easing\easeInBack.js
Type: files; Name: {app}\presets\expressions\easing\easeInBounce.js
Type: files; Name: {app}\presets\expressions\easing\easeInCirc.js
Type: files; Name: {app}\presets\expressions\easing\easeInCubic.js
Type: files; Name: {app}\presets\expressions\easing\easeInElastic.js
Type: files; Name: {app}\presets\expressions\easing\easeInExpo.js
Type: files; Name: {app}\presets\expressions\easing\easeInOutBack.js
Type: files; Name: {app}\presets\expressions\easing\easeInOutBounce.js
Type: files; Name: {app}\presets\expressions\easing\easeInOutCirc.js
Type: files; Name: {app}\presets\expressions\easing\easeInOutCubic.js
Type: files; Name: {app}\presets\expressions\easing\easeInOutElastic.js
Type: files; Name: {app}\presets\expressions\easing\easeInOutExpo.js
Type: files; Name: {app}\presets\expressions\easing\easeInOutQuad.js
Type: files; Name: {app}\presets\expressions\easing\easeInOutQuart.js
Type: files; Name: {app}\presets\expressions\easing\easeInOutQuint.js
Type: files; Name: {app}\presets\expressions\easing\easeInOutSine.js
Type: files; Name: {app}\presets\expressions\easing\easeInQuad.js
Type: files; Name: {app}\presets\expressions\easing\easeInQuart.js
Type: files; Name: {app}\presets\expressions\easing\easeInQuint.js
Type: files; Name: {app}\presets\expressions\easing\easeInSine.js
Type: files; Name: {app}\presets\expressions\easing\easeOutBack.js
Type: files; Name: {app}\presets\expressions\easing\easeOutBounce.js
Type: files; Name: {app}\presets\expressions\easing\easeOutCirc.js
Type: files; Name: {app}\presets\expressions\easing\easeOutCubic.js
Type: files; Name: {app}\presets\expressions\easing\easeOutElastic.js
Type: files; Name: {app}\presets\expressions\easing\easeOutExpo.js
Type: files; Name: {app}\presets\expressions\easing\easeOutQuad.js
Type: files; Name: {app}\presets\expressions\easing\easeOutQuart.js
Type: files; Name: {app}\presets\expressions\easing\easeOutQuint.js
Type: files; Name: {app}\presets\expressions\easing\easeOutSine.js
Type: files; Name: {app}\docs\LICENSE.md
Type: files; Name: {app}\docs\README-easing.md
Type: files; Name: {app}\docs\README.md
Type: files; Name: {app}\docs\ffmpeg\COPYING.GPLv3
Type: files; Name: {app}\docs\ffmpeg\CREDITS
Type: files; Name: {app}\docs\ffmpeg\gcc\COPYING.LIB
Type: files; Name: {app}\docs\ffmpeg\gcc\COPYING.RUNTIME
Type: files; Name: {app}\docs\ffmpeg\l-smash\LICENSE
Type: files; Name: {app}\docs\ffmpeg\lame\COPYING
Type: files; Name: {app}\docs\ffmpeg\lame\LICENSE
Type: files; Name: {app}\docs\ffmpeg\lame\README
Type: files; Name: {app}\docs\ffmpeg\libaom\AUTHORS
Type: files; Name: {app}\docs\ffmpeg\libaom\LICENSE
Type: files; Name: {app}\docs\ffmpeg\libaom\PATENTS
Type: files; Name: {app}\docs\ffmpeg\libde265\AUTHORS
Type: files; Name: {app}\docs\ffmpeg\libde265\COPYING
Type: files; Name: {app}\docs\ffmpeg\libde265\README
Type: files; Name: {app}\docs\ffmpeg\libheif\COPYING
Type: files; Name: {app}\docs\ffmpeg\libheif\README.md
Type: files; Name: {app}\docs\ffmpeg\libogg\AUTHORS
Type: files; Name: {app}\docs\ffmpeg\libogg\COPYING
Type: files; Name: {app}\docs\ffmpeg\libogg\README.md
Type: files; Name: {app}\docs\ffmpeg\libtheora\AUTHORS
Type: files; Name: {app}\docs\ffmpeg\libtheora\COPYING
Type: files; Name: {app}\docs\ffmpeg\libtheora\LICENSE
Type: files; Name: {app}\docs\ffmpeg\libtheora\README
Type: files; Name: {app}\docs\ffmpeg\libvorbis\AUTHORS
Type: files; Name: {app}\docs\ffmpeg\libvorbis\COPYING
Type: files; Name: {app}\docs\ffmpeg\libvpx\AUTHORS
Type: files; Name: {app}\docs\ffmpeg\libvpx\LICENSE
Type: files; Name: {app}\docs\ffmpeg\libvpx\PATENTS
Type: files; Name: {app}\docs\ffmpeg\libvpx\README
Type: files; Name: {app}\docs\ffmpeg\x265\COPYING
Type: files; Name: {app}\docs\ffmpeg\x265\readme.rst
Type: files; Name: {app}\docs\ffmpeg\xvidcore\AUTHORS
Type: files; Name: {app}\docs\ffmpeg\xvidcore\LICENSE
Type: files; Name: {app}\docs\ffmpeg\xvidcore\README
Type: files; Name: {app}\docs\ffmpeg\zlib\LICENSE
Type: files; Name: {app}\docs\ffmpeg\zlib\README
Type: files; Name: {app}\docs\qscintilla\LICENSE
Type: files; Name: {app}\docs\qscintilla\README.doc
Type: files; Name: {app}\docs\qt\LICENSE
Type: files; Name: {app}\docs\qt\LICENSE.GPL3-EXCEPT
Type: files; Name: {app}\docs\qt\LICENSE.txt
Type: files; Name: {app}\docs\qt\ThirdPartySoftware_Listing.txt
Type: files; Name: {app}\docs\skia\AUTHORS
Type: files; Name: {app}\docs\skia\LICENSE
Type: files; Name: {app}\docs\skia\README
Type: files; Name: {app}\svgo-win.exe
Type: files; Name: {app}\svgo-license.txt
