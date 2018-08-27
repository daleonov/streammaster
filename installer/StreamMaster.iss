[Setup]
AppName=StreamMaster
AppVersion=2.0.0
DefaultDirName={pf}\Daniel Leonov Plugs StreamMaster
DefaultGroupName=Daniel Leonov Plugs
Compression=lzma2
SolidCompression=yes
OutputDir=.\
ArchitecturesInstallIn64BitMode=x64
OutputBaseFilename=StreamMaster Installer
LicenseFile=license.rtf
SetupLogging=yes   
AppPublisher=Daniel Leonov Plugs
AppPublisherURL=https://danielleonovplugs.com
AppSupportURL=https://danielleonovplugs.com/feedback
AppUpdatesURL=https://danielleonovplugs.com/streammaster    
WizardImageFile=install-55x55.bmp

[Types]
Name: "full"; Description: "Full installation"
Name: "custom"; Description: "Custom installation"; Flags: iscustom

[Components]
Name: "vst2_32"; Description: "32-bit VST2 Plugin (.dll)"; Types: full custom
Name: "vst2_64"; Description: "64-bit VST2 Plugin (.dll)"; Types: full custom; Check: Is64BitInstallMode
Name: "aax_64"; Description: "64-bit AAX Plugin (.aaxplugin)"; Types: full custom; Check: Is64BitInstallMode
Name: "manual"; Description: "User guide"; Types: full custom

[Files]
Source: "..\build-win\vst2\Win32\bin\StreamMaster.dll"; DestDir: "{code:GetVST2Dir_32}"; Flags: ignoreversion; Components: vst2_32; Check: Is64BitInstallMode
Source: "..\build-win\vst2\x64\bin\StreamMaster.dll"; DestDir: "{code:GetVST2Dir_64}"; Flags: ignoreversion; Components: vst2_64; Check: Is64BitInstallMode
Source: "C:\Program Files\Common Files\Avid\Audio\Plug-Ins\StreamMaster.aaxplugin\Contents\x64\StreamMaster.aaxplugin"; DestDir: "{cf}\Avid\Audio\Plug-Ins\SubKicker.aaxplugin\Contents\x64\"; Flags: ignoreversion recursesubdirs; Components: aax_64

Source: "..\manual\StreamMaster 2.0.0 Manual.pdf"; DestDir: "{app}"; Components: manual

[Icons]
Name: "{group}\User guide"; Filename: "{app}\StreamMaster 2.0.0 Manual.pdf"
;Name: "{group}\readme"; Filename: "{app}\readme.rtf"
Name: "{group}\Uninstall StreamMaster"; Filename: "{app}\unins000.exe"

;[Dirs]

[Code]
var
  OkToCopyLog : Boolean;
  VST2DirPage_32: TInputDirWizardPage;
  VST2DirPage_64: TInputDirWizardPage;

procedure InitializeWizard;
begin
  if IsWin64 then begin
    VST2DirPage_64 := CreateInputDirPage(wpSelectDir,
    'Confirm 64-Bit VST2 Plugin Directory', '',
    'Select the folder in which setup should install the 64-bit VST2 Plugin, then click Next.',
    False, '');
    VST2DirPage_64.Add('');
    VST2DirPage_64.Values[0] := ExpandConstant('{reg:HKLM\SOFTWARE\VST,VSTPluginsPath|{pf}\Steinberg\VSTPlugins}\');

    VST2DirPage_32 := CreateInputDirPage(wpSelectDir,
      'Confirm 32-Bit VST2 Plugin Directory', '',
      'Select the folder in which setup should install the 32-bit VST2 Plugin, then click Next.',
      False, '');
    VST2DirPage_32.Add('');
    VST2DirPage_32.Values[0] := ExpandConstant('{reg:HKLM\SOFTWARE\WOW6432NODE\VST,VSTPluginsPath|{pf32}\Steinberg\VSTPlugins}\');
  end else begin
    VST2DirPage_32 := CreateInputDirPage(wpSelectDir,
      'Confirm 32-Bit VST2 Plugin Directory', '',
      'Select the folder in which setup should install the 32-bit VST2 Plugin, then click Next.',
      False, '');
    VST2DirPage_32.Add('');
    VST2DirPage_32.Values[0] := ExpandConstant('{reg:HKLM\SOFTWARE\VST,VSTPluginsPath|{pf}\Steinberg\VSTPlugins}\');
  end;
end;

function GetVST2Dir_32(Param: String): String;
begin
  Result := VST2DirPage_32.Values[0]
end;

function GetVST2Dir_64(Param: String): String;
begin
  Result := VST2DirPage_64.Values[0]
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssDone then
    OkToCopyLog := True;
end;

procedure DeinitializeSetup();
begin
  if OkToCopyLog then
    FileCopy (ExpandConstant ('{log}'), ExpandConstant ('{app}\InstallationLogFile.log'), FALSE);
  RestartReplace (ExpandConstant ('{log}'), '');
end;

[UninstallDelete]
Type: files; Name: "{app}\InstallationLogFile.log"