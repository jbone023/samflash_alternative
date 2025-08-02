# SamFlash Alternative Installation Script for Windows
# Run as Administrator

param(
    [string]$InstallPath = "$env:ProgramFiles\SamFlash Alternative",
    [switch]$CreateDesktopShortcut = $true,
    [switch]$AddToPath = $true
)

Write-Host "SamFlash Alternative Installation Script" -ForegroundColor Green
Write-Host "=======================================" -ForegroundColor Green

# Check if running as administrator
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Error "This script must be run as Administrator. Exiting..."
    exit 1
}

# Create installation directory
Write-Host "Creating installation directory: $InstallPath"
New-Item -ItemType Directory -Force -Path $InstallPath | Out-Null

# Copy application files (assuming build output is in Release folder)
$SourcePath = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$BuildPath = "$SourcePath\build\Release"

if (Test-Path $BuildPath) {
    Write-Host "Copying application files..."
    Copy-Item "$BuildPath\*" -Destination $InstallPath -Recurse -Force
} else {
    Write-Warning "Build directory not found. Please build the application first."
}

# Copy assets and documentation
Write-Host "Copying assets and documentation..."
Copy-Item "$SourcePath\assets\*" -Destination "$InstallPath\assets" -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item "$SourcePath\docs\*" -Destination "$InstallPath\docs" -Recurse -Force -ErrorAction SilentlyContinue

# Install Visual C++ Redistributable if needed
Write-Host "Checking Visual C++ Redistributable..."
$VCRedistInstalled = Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*" | 
                     Where-Object { $_.DisplayName -like "*Visual C++*Redistributable*" }

if (-not $VCRedistInstalled) {
    Write-Host "Visual C++ Redistributable not found. Please install it manually."
    Write-Host "Download from: https://aka.ms/vs/17/release/vc_redist.x64.exe"
}

# Create desktop shortcut
if ($CreateDesktopShortcut) {
    Write-Host "Creating desktop shortcut..."
    $WshShell = New-Object -comObject WScript.Shell
    $Shortcut = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\SamFlash Alternative.lnk")
    $Shortcut.TargetPath = "$InstallPath\SamFlashGUI.exe"
    $Shortcut.WorkingDirectory = $InstallPath
    $Shortcut.IconLocation = "$InstallPath\assets\icon.ico"
    $Shortcut.Description = "SamFlash Alternative - Flash Memory Programming Tool"
    $Shortcut.Save()
}

# Add to Windows PATH
if ($AddToPath) {
    Write-Host "Adding to system PATH..."
    $CurrentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")
    if ($CurrentPath -notlike "*$InstallPath*") {
        [Environment]::SetEnvironmentVariable("Path", "$CurrentPath;$InstallPath", "Machine")
        Write-Host "Added to PATH. Restart your command prompt to use CLI tools."
    }
}

# Register file associations
Write-Host "Registering file associations..."
$Extensions = @(".bin", ".hex", ".elf")

foreach ($Ext in $Extensions) {
    $RegPath = "HKCR:\$Ext"
    if (-not (Test-Path $RegPath)) {
        New-Item -Path $RegPath -Value "SamFlashFirmware" -Force | Out-Null
    }
}

$RegPath = "HKCR:\SamFlashFirmware"
if (-not (Test-Path $RegPath)) {
    New-Item -Path $RegPath -Value "Firmware File" -Force | Out-Null
    New-Item -Path "$RegPath\DefaultIcon" -Value "$InstallPath\assets\firmware.ico" -Force | Out-Null
    New-Item -Path "$RegPath\shell\open\command" -Value "`"$InstallPath\SamFlashGUI.exe`" `"%1`"" -Force | Out-Null
}

# Create uninstaller
Write-Host "Creating uninstaller..."
$UninstallScript = @"
# SamFlash Alternative Uninstaller
Write-Host "Uninstalling SamFlash Alternative..."

# Remove installation directory
Remove-Item "$InstallPath" -Recurse -Force -ErrorAction SilentlyContinue

# Remove from PATH
`$CurrentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")
`$NewPath = `$CurrentPath -replace [regex]::Escape(";$InstallPath"), ""
[Environment]::SetEnvironmentVariable("Path", `$NewPath, "Machine")

# Remove desktop shortcut
Remove-Item "`$env:USERPROFILE\Desktop\SamFlash Alternative.lnk" -ErrorAction SilentlyContinue

# Remove registry entries
Remove-Item "HKCR:\.bin" -ErrorAction SilentlyContinue
Remove-Item "HKCR:\.hex" -ErrorAction SilentlyContinue
Remove-Item "HKCR:\.elf" -ErrorAction SilentlyContinue
Remove-Item "HKCR:\SamFlashFirmware" -Recurse -ErrorAction SilentlyContinue

Write-Host "Uninstallation complete."
"@

$UninstallScript | Out-File -FilePath "$InstallPath\Uninstall.ps1" -Encoding UTF8

Write-Host ""
Write-Host "Installation completed successfully!" -ForegroundColor Green
Write-Host "Application installed to: $InstallPath"
Write-Host "To uninstall, run: $InstallPath\Uninstall.ps1"
Write-Host ""
Write-Host "You can now run:" -ForegroundColor Yellow
Write-Host "  - GUI: $InstallPath\SamFlashGUI.exe"
Write-Host "  - CLI: samflash-cli (if added to PATH)"
