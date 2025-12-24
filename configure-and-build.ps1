param(
    [ValidateSet("debug", "release")]
    [string]$Config,

    [ValidateSet("DX12", "VULKAN")]
    [string]$Renderer
)

$env:VCPKG_ROOT = "C:\dev\vcpkg"

# Determine preset
$preset = if ($Config -eq "release") { "windows-msvc-release" } else { "windows-msvc-debug" }

# Set default config if not provided
if ([string]::IsNullOrEmpty($Config)) {
    $Config = "debug"
}

# Set default renderer if not provided (Windows default is DX12)
if ([string]::IsNullOrEmpty($Renderer)) {
    $Renderer = "DX12"
}

Write-Host "==============================================" -ForegroundColor Magenta
Write-Host "Configuration: $Config" -ForegroundColor Magenta
Write-Host "Preset:        $preset" -ForegroundColor Magenta
Write-Host "Renderer:      $Renderer" -ForegroundColor Magenta
Write-Host "==============================================" -ForegroundColor Magenta

# Configure
# Note: We pass -DSOLARC_RENDERER explicitly to override the preset default if needed
cmake --preset $preset "-DSOLARC_RENDERER=$Renderer"
if ($LASTEXITCODE -ne 0) {
    Write-Host "Configuration failed!" -ForegroundColor Red
    exit 1
}

# Build
Write-Host "Building..." -ForegroundColor Cyan
cmake --build --preset $preset
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    exit 1
}

# Determine Output Path based on CMakeLists logic: ${CMAKE_BINARY_DIR}/out/${CONFIG}/bin
# CMake capitalizes the first letter of the config in folders (Debug/Release)
$configFolder = (Get-Culture).TextInfo.ToTitleCase($Config)
$binaryDir = "$PSScriptRoot/build/$preset/out/$configFolder/bin"

Write-Host "Build successful!" -ForegroundColor Green
Write-Host "Executables are in: $binaryDir" -ForegroundColor Yellow

# List executables
if (Test-Path $binaryDir) {
    Get-ChildItem -Path $binaryDir -Include *.exe -Recurse | ForEach-Object {
        Write-Host "Found: $($_.Name)" -ForegroundColor Blue
    }
} else {
    Write-Host "Warning: Binary directory not found at expected path: $binaryDir" -ForegroundColor DarkYellow
}