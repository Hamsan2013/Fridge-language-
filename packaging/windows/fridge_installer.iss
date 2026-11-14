[Setup]
AppName=FRIDGE Programming Language
AppVersion={#MyAppVersion}
AppPublisher=Hamsan A
DefaultDirName={autopf}\FRIDGE
DefaultGroupName=FRIDGE
OutputDir=..\..\dist
OutputBaseFilename=FRIDGE-Setup-v0.5.0
SetupIconFile=..\..\resources\fridge.ico
Compression=lzma2
SolidCompression=yes
ChangesEnvironment=yes

[Files]
Source: "..\..\dist\bin\fridge.exe"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\resources\fridge.ico"; DestDir: "{app}\resources"; Flags: ignoreversion
Source: "..\..\packaging\windows\register_frg_icon.bat"; DestDir: "{app}\bin"; Flags: ignoreversion
Source: "..\..\tests\golden\*"; DestDir: "{app}\examples"; Flags: recursesubdirs
Source: "..\..\README.md"; DestDir: "{app}"; Flags: isreadme

[Registry]
; Register .frg and .uzip file associations and icon
Root: HKCR; Subkey: ".frg"; ValueType: string; ValueData: "FridgeSourceFile"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "FridgeSourceFile"; ValueType: string; ValueData: "FRIDGE Source Script"; Flags: uninsdeletekey
Root: HKCR; Subkey: "FridgeSourceFile\DefaultIcon"; ValueType: string; ValueData: "{app}\resources\fridge.ico,0"
Root: HKCR; Subkey: "FridgeSourceFile\shell\open\command"; ValueType: string; ValueData: """{app}\bin\fridge.exe"" run ""%1"""

Root: HKCR; Subkey: ".uzip"; ValueType: string; ValueData: "FridgePackageFile"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "FridgePackageFile"; ValueType: string; ValueData: "FRIDGE Application Package"; Flags: uninsdeletekey
Root: HKCR; Subkey: "FridgePackageFile\DefaultIcon"; ValueType: string; ValueData: "{app}\resources\fridge.ico,0"
Root: HKCR; Subkey: "FridgePackageFile\shell\open\command"; ValueType: string; ValueData: """{app}\bin\fridge.exe"" run ""%1"""

; Optional PATH registration
[Tasks]
Name: envPath; Description: "Add FRIDGE to system PATH environment variable"

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
var
  Path: string;
begin
  if (CurStep = ssPostInstall) and WizardIsTaskSelected('envPath') then
  begin
    if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment', 'Path', Path) then
    begin
      if Pos(ExpandConstant('{app}\bin'), Path) = 0 then
      begin
        Path := Path + ';' + ExpandConstant('{app}\bin');
        RegWriteStringValue(HKEY_LOCAL_MACHINE, 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment', 'Path', Path);
      end;
    end;
  end;
end;
