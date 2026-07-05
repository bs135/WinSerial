# -----------------------------------------------------------------------------
# WINSERIAL: REMOVE WINDOWS TERMINAL PROFILE SCRIPT
# -----------------------------------------------------------------------------

# Resolve the standard path to the Windows Terminal settings.json file
$settingsPath = "$env:LOCALAPPDATA\Packages\Microsoft.WindowsTerminal_8wekyb3d8bbwe\LocalState\settings.json"

# Verify if Windows Terminal is installed on the current system
if (-not (Test-Path $settingsPath)) {
    Write-Host "Windows Terminal is not installed. Skipping profile removal."
    exit
}

# Read the file content line by line
$lines = Get-Content -Path $settingsPath
$outLines = @()
$skipMode = $false

# Algorithm: Scan line-by-line. If the WinSerial profile block is detected,
# enable "skip mode" to omit those lines from the new configuration output.
for ($i = 0; $i -lt $lines.Count; $i++) {
    $line = $lines[$i]

    # Detect the start of a profile block (a line containing only '{' and optional whitespaces)
    if ($line -match '^\s*\{\s*$') {
        
        # Look-ahead: Check subsequent lines to determine if this is the WinSerial profile
        $isWinSerial = $false
        for ($j = $i + 1; $j -lt $lines.Count; $j++) {
            if ($lines[$j] -match '"name":\s*"WinSerial"') {
                $isWinSerial = $true
                break
            }
            if ($lines[$j] -match '^\s*\}\s*,?\s*$') {
                break # Reached the end of the current profile block without a match
            }
        }

        if ($isWinSerial) {
            $skipMode = $true
            continue # Skip appending the opening brace line
        }
    }

    if ($skipMode) {
        # Detect the end of the profile block currently being skipped
        if ($line -match '^\s*\}\s*,?\s*$') {
            $skipMode = $false
        }
        continue # Skip the internal lines of the WinSerial profile block
    }

    # If not within the WinSerial profile block, retain the current line
    $outLines += $line
}

# Reconstruct the JSON content string from the retained lines
$newContent = $outLines -join "`r`n"

# Handle potential trailing comma syntax errors in JSON.
# If the removed WinSerial block was the last array element, the preceding element
# might now have a trailing comma (e.g., '}, ]').
# This regex locates a '},' adjacent to a closing bracket ']' and removes the comma.
$newContent = $newContent -replace '(?s)\}(\s*),\s*\]', "}`$1]"

# Save the updated configuration back to settings.json using UTF-8 encoding
Set-Content -Path $settingsPath -Value $newContent -Encoding UTF8
Write-Host "Successfully removed WinSerial profile from Windows Terminal."
