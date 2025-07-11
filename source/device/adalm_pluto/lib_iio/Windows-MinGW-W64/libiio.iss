[Setup]
AppId={{D386A5F6-D38D-4738-94A2-E163DC1896F1}
AppName="Libiio"
AppVersion="0.26"
AppPublisher="Analog Devices, Inc."
AppPublisherURL="http://www.analog.com"
AppSupportURL="http://www.analog.com"
AppUpdatesURL="http://www.analog.com"
AppCopyright="Copyright 2015-2024 ADI and other contributors"
CreateAppDir=no
LicenseFile="D:\a\1\s\COPYING.txt"
OutputBaseFilename=libiio-setup
OutputDir="C:\"
Compression=lzma
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "catalan"; MessagesFile: "compiler:Languages\Catalan.isl"
Name: "corsican"; MessagesFile: "compiler:Languages\Corsican.isl"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "hebrew"; MessagesFile: "compiler:Languages\Hebrew.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"

[Files]
Source: "D:\a\1\a\Windows-VS-2019-x64\libiio.dll"; DestDir: "{sys}"; Check: Is64BitInstallMode; Flags: replacesameversion
Source: "D:\a\1\a\Windows-VS-2019-x64\*.exe"; DestDir: "{sys}"; Check: Is64BitInstallMode; Flags: replacesameversion
Source: "D:\a\1\a\Windows-VS-2019-x64\libiio.lib"; DestDir: "{commonpf32}\Microsoft Visual Studio 12.0\VC\lib\amd64"; Check: Is64BitInstallMode
Source: "D:\a\1\a\Windows-VS-2019-x64\iio.h"; DestDir: "{commonpf32}\Microsoft Visual Studio 12.0\VC\include"
Source: "D:\a\1\a\Windows-VS-2019-x64\libxml2.dll"; DestDir: "{sys}"; Check: Is64BitInstallMode; Flags: onlyifdoesntexist
Source: "D:\a\1\a\Windows-VS-2019-x64\libusb-1.0.dll"; DestDir: "{sys}"; Check: Is64BitInstallMode; Flags: onlyifdoesntexist
Source: "D:\a\1\a\Windows-VS-2019-x64\libserialport-0.dll"; DestDir: "{sys}"; Check: Is64BitInstallMode; Flags: onlyifdoesntexist
Source: "D:\a\1\a\Windows-VS-2019-x64\libiio-sharp.dll"; DestDir: "{commoncf}\libiio"; Flags: replacesameversion
Source: "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Redist\MSVC\14.29.30133\x64\Microsoft.VC142.CRT\msvcp140.dll"; DestDir: "{sys}"; Check: Is64BitInstallMode; Flags: onlyifdoesntexist
Source: "C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Redist\MSVC\14.29.30133\x64\Microsoft.VC142.CRT\vcruntime140.dll"; DestDir: "{sys}"; Check: Is64BitInstallMode; Flags: onlyifdoesntexist
