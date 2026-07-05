# -----------------------------------------------------------------------------
# WINSERIAL: ADD WINDOWS TERMINAL PROFILE SCRIPT
# -----------------------------------------------------------------------------

param(
    # The installation directory path, passed by Inno Setup during execution
    [Parameter(Mandatory=$true)]
    [string]$InstallPath
)

# Resolve the standard path to the Windows Terminal settings.json file
$settingsPath = "$env:LOCALAPPDATA\Packages\Microsoft.WindowsTerminal_8wekyb3d8bbwe\LocalState\settings.json"

# Verify if Windows Terminal is installed on the current system
if (-not (Test-Path $settingsPath)) {
    Write-Host "Windows Terminal is not installed on this machine. Skipping profile addition."
    exit
}

# Read the raw content of the settings.json file
$content = Get-Content -Path $settingsPath -Raw

# Check for an existing WinSerial profile to prevent duplicates during upgrades/reinstalls
if ($content -match '"name":\s*"WinSerial"') {
    Write-Host "WinSerial profile already exists in Windows Terminal. Skipping."
    exit
}

# 1. Construct the absolute path to the WinSerial executable
$exePath = Join-Path -Path $InstallPath -ChildPath "WinSerial.exe"

# 2. Escape backslashes (\ to \\) to ensure valid JSON syntax
$exePathEscaped = $exePath -replace '\\', '\\'

# 3. Generate a unique GUID for the new profile
$guid = "{$( [guid]::NewGuid().ToString() )}"

# 4. Define the JSON snippet for the WinSerial profile
# Note: Includes a trailing comma to properly separate it from the existing profiles
$profileSnippet = @"
            {
                "closeOnExit": "never",
                "commandline": "$exePathEscaped",
                "guid": "$guid",
                "hidden": false,
                "icon": "$exePathEscaped",
                "name": "WinSerial"
            },
"@

# 5. Locate the "list": [ array in settings.json and inject the new profile
# The regex targets the "list" array declaration, accounting for potential whitespaces
if ($content -match '("list":\s*\[)') {
    # Inject the profile snippet immediately after the array's opening bracket
    $newContent = $content -replace '("list":\s*\[)', "`$1`r`n$profileSnippet"
    
    # Save the updated configuration back to the file using UTF-8 encoding
    Set-Content -Path $settingsPath -Value $newContent -Encoding UTF8
    Write-Host "Successfully added WinSerial profile to Windows Terminal."
} else {
    Write-Warning "Failed to locate the 'list' array in settings.json. Skipping profile addition."
}
