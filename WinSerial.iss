; -----------------------------------------------------------------------------
; WINSERIAL INNO SETUP SCRIPT
; -----------------------------------------------------------------------------

#define MyAppName "WinSerial"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "ChipFC Team"
#define MyAppExeName "WinSerial.exe"

[Setup]
; --- App Information ---
AppId={{AAB33D77-6BAE-4620-837D-BAD09C437110}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppCopyright=© 2026 {#MyAppPublisher}
AppPublisher={#MyAppPublisher}
AppPublisherURL=https://github.com/bs135/WinSerial

; --- Installation Settings ---
DefaultDirName={autopf}\{#MyAppName}
DisableProgramGroupPage=yes

; Notify the OS to update environment variables after installation
ChangesEnvironment=yes

; --- Compiler Output Settings ---
OutputDir=.\build\Installer
OutputBaseFilename=WinSerial-setup
Compression=lzma
SolidCompression=yes
WizardStyle=modern
LicenseFile=.\LICENSE
SetupIconFile=.\assets\WinSerial-setup.ico

; --- Architecture Settings (Force 64-bit mode) ---
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64

; --- Uninstaller Settings ---
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\{#MyAppExeName}

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
; Core executable file
Source: "build\Release\{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
; PowerShell scripts for Windows Terminal integration
Source: "scripts\Add-TerminalProfile.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "scripts\Remove-TerminalProfile.ps1"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Registry]
; Add the installation directory to the user's PATH environment variable
Root: HKCU; Subkey: "Environment"; ValueType: expandsz; ValueName: "Path"; ValueData: "{olddata};{app}"; Check: NeedsAddPath('{app}')

[Run]
; 1. Silently execute the PowerShell script to add the Windows Terminal profile
Filename: "powershell.exe"; \
    Parameters: "-ExecutionPolicy Bypass -WindowStyle Hidden -File ""{app}\Add-TerminalProfile.ps1"" -InstallPath ""{app}"""; \
    StatusMsg: "Adding Windows Terminal profile..."; \
    Flags: runhidden waituntilterminated

; 2. Launch WinSerial inside Windows Terminal as the original user (not as Administrator)
Filename: "wt.exe"; Parameters: "-p ""WinSerial"""; Description: "Launch {#MyAppName} in Windows Terminal"; Flags: nowait postinstall skipifsilent runasoriginaluser

[UninstallRun]
; Silently execute the PowerShell script to remove the Windows Terminal profile during uninstallation
Filename: "powershell.exe"; \
    Parameters: "-ExecutionPolicy Bypass -WindowStyle Hidden -File ""{app}\Remove-TerminalProfile.ps1"""; \
    RunOnceId: "RemoveTerminalProfile"; \
    StatusMsg: "Removing Windows Terminal profile..."; \
    Flags: runhidden waituntilterminated

[Code]
// Checks if the application path already exists in the system PATH environment variable
function NeedsAddPath(Param: string): boolean;
var
  OrigPath: string;
  ExpandedParam: string;
begin
  // Expand the {app} constant to the actual installation path
  ExpandedParam := ExpandConstant(Param);

  if not RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', OrigPath) then
  begin
    Result := True;
    exit;
  end;

  // Pad both strings with ';' and convert to uppercase for accurate case-insensitive matching
  OrigPath := ';' + Uppercase(OrigPath) + ';';
  ExpandedParam := ';' + Uppercase(ExpandedParam) + ';';

  // Return True if the path is not found in the original PATH string
  Result := Pos(ExpandedParam, OrigPath) = 0;
end;

// Removes the application path from the system PATH environment variable during uninstallation
procedure RemovePath(PathToRemove: string);
var
  OrigPath: string;
  ExpandedPath: string;
  PosIndex: Integer;
begin
  // 1. Expand the {app} constant to the actual installation path (e.g., C:\Program Files\WinSerial)
  ExpandedPath := ExpandConstant(PathToRemove);

  // 2. Retrieve the current PATH variable for the user
  if RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', OrigPath) then
  begin
    // Pad with ';' to ensure exact matching of the path string
    OrigPath := ';' + OrigPath + ';';
    ExpandedPath := ';' + ExpandedPath + ';';

    // 3. Find the position of the path string (case-insensitive)
    PosIndex := Pos(Uppercase(ExpandedPath), Uppercase(OrigPath));

    // If found, proceed to remove it
    if PosIndex > 0 then
    begin
      // Delete the matched path string, leaving one trailing ';'
      Delete(OrigPath, PosIndex, Length(ExpandedPath) - 1);

      // Clean up any duplicated semicolons ';;'
      while Pos(';;', OrigPath) > 0 do
        StringChangeEx(OrigPath, ';;', ';', True);

      // Remove leading or trailing semicolons if they exist
      if Copy(OrigPath, 1, 1) = ';' then
        Delete(OrigPath, 1, 1);
      if Copy(OrigPath, Length(OrigPath), 1) = ';' then
        Delete(OrigPath, Length(OrigPath), 1);

      // 4. Overwrite the cleaned PATH back into the Registry
      RegWriteStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', OrigPath);
    end;
  end;
end;

// Inno Setup built-in event handler.
// Automatically triggered during the uninstallation process.
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  // Trigger the PATH cleanup just before the application files are uninstalled
  if CurUninstallStep = usUninstall then
  begin
    RemovePath('{app}');
  end;
end;
