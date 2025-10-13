# TimeStick Driver Build and Test Script
# Build and test the TimeStick Windows driver

param(
    [switch]$Build,
    [switch]$Test,
    [switch]$Install,
    [switch]$Uninstall,
    [switch]$Help
)

# Script configuration
$DriverName = "TimeStick"
$DriverSourceDir = "."
$TestDir = "tests"
$BuildDir = "build"
$DriverFile = "$DriverName.sys"
$InfFile = "$DriverName.inf"

# Colors for output
$Green = "Green"
$Red = "Red"
$Yellow = "Yellow"
$Blue = "Blue"

# Script-scoped variables
$script:msvcVersion = $null
$script:wdkVersion = $null
$script:vsPath = $null
$script:wdkPath = $null

# Helper functions for path resolution
function Get-LatestWdkPath {
    $wdkBase = "${env:ProgramFiles(x86)}\Windows Kits\10\bin"
    
    if (-not (Test-Path $wdkBase)) {
        return $null
    }
    
    $latestVersion = Get-ChildItem $wdkBase -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
        Sort-Object Name -Descending |
        Select-Object -First 1
    
    if ($latestVersion) {
        $script:wdkVersion = $latestVersion.Name
        return "$($latestVersion.FullName)\x64"
    }
    
    return $null
}

function Get-WdkIncludePath {
    $wdkBase = "${env:ProgramFiles(x86)}\Windows Kits\10\Include"
    
    if (-not (Test-Path $wdkBase)) {
        return $null
    }
    
    $latestVersion = Get-ChildItem $wdkBase -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
        Sort-Object Name -Descending |
        Select-Object -First 1
    
    if ($latestVersion) {
        return $latestVersion.FullName
    }
    
    return $null
}

function Get-WdkLibPath {
    $wdkBase = "${env:ProgramFiles(x86)}\Windows Kits\10\Lib"
    
    if (-not (Test-Path $wdkBase)) {
        return $null
    }
    
    $latestVersion = Get-ChildItem $wdkBase -Directory -ErrorAction SilentlyContinue |
        Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
        Sort-Object Name -Descending |
        Select-Object -First 1
    
    if ($latestVersion) {
        return $latestVersion.FullName
    }
    
    return $null
}

function Get-LatestMsvcVersion {
    $msvcBase = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC"
    
    if (-not (Test-Path $msvcBase)) {
        # Try Community edition
        $msvcBase = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"
        if (-not (Test-Path $msvcBase)) {
            return $null
        }
    }
    
    $latestVersion = Get-ChildItem $msvcBase -Directory -ErrorAction SilentlyContinue |
        Sort-Object Name -Descending |
        Select-Object -First 1
    
    if ($latestVersion) {
        return $latestVersion.Name
    }
    
    return $null
}

function Get-MsvcBasePath {
    $buildToolsPath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC"
    if (Test-Path $buildToolsPath) {
        return $buildToolsPath
    }
    
    $communityPath = "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC"
    if (Test-Path $communityPath) {
        return $communityPath
    }
    
    return $null
}

function Write-ColoredOutput {
    param([string]$Message, [string]$Color = "White")
    Write-Host $Message -ForegroundColor $Color
}

function Show-Help {
    Write-ColoredOutput "TimeStick Driver Build and Test Script" $Blue
    Write-ColoredOutput "=====================================" $Blue
    Write-ColoredOutput ""
    Write-ColoredOutput "Usage: .\build.ps1 [options]" $Green
    Write-ColoredOutput ""
    Write-ColoredOutput "Options:" $Green
    Write-ColoredOutput "  -Build      Build the driver" $Green
    Write-ColoredOutput "  -Test       Run test suite" $Green
    Write-ColoredOutput "  -Install    Install the driver" $Green
    Write-ColoredOutput "  -Uninstall  Uninstall the driver" $Green
    Write-ColoredOutput "  -Help       Show this help" $Green
    Write-ColoredOutput ""
    Write-ColoredOutput "Examples:" $Yellow
    Write-ColoredOutput "  .\build.ps1 -Build" $Yellow
    Write-ColoredOutput "  .\build.ps1 -Build -Test" $Yellow
    Write-ColoredOutput "  .\build.ps1 -Install" $Yellow
}

function Test-Prerequisites {
    Write-ColoredOutput "Checking prerequisites..." $Blue

    # Check if Visual Studio Build Tools are installed
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsPath) {
            Write-ColoredOutput "✓ Visual Studio Build Tools found at $vsPath" $Green
        } else {
            Write-ColoredOutput "✗ Visual Studio Build Tools not found" $Red
            Write-ColoredOutput "  Please install Visual Studio Build Tools or Visual Studio 2022" $Red
            exit 1
        }
    } else {
        Write-ColoredOutput "✗ vswhere.exe not found" $Red
        Write-ColoredOutput "  Please install Visual Studio 2022" $Red
        exit 1
    }

    # Initialize paths
    $script:wdkPath = Get-LatestWdkPath
    $script:msvcVersion = Get-LatestMsvcVersion
    
    if (-not $script:wdkPath) {
        Write-ColoredOutput "✗ Windows Driver Kit not found" $Red
        Write-ColoredOutput "  Please install Windows Driver Kit 10" $Red
        exit 1
    } else {
        Write-ColoredOutput "✓ Windows Driver Kit found (version $script:wdkVersion)" $Green
    }
    
    if (-not $script:msvcVersion) {
        Write-ColoredOutput "✗ MSVC compiler not found" $Red
        Write-ColoredOutput "  Please install Visual Studio 2022 with C++ tools" $Red
        exit 1
    } else {
        Write-ColoredOutput "✓ MSVC compiler found (version $script:msvcVersion)" $Green
    }

    # Check if source files exist
    if (Test-Path "$DriverSourceDir\driver.c") {
        Write-ColoredOutput "✓ Driver source files found" $Green
    } else {
        Write-ColoredOutput "✗ Driver source files not found" $Red
        Write-ColoredOutput "  Expected to find driver.c in current directory" $Red
        exit 1
    }
}

function Build-Driver {
    Write-ColoredOutput "Building TimeStick driver..." $Blue

    # Create build directory if it doesn't exist
    if (-not (Test-Path $BuildDir)) {
        New-Item -ItemType Directory -Path $BuildDir | Out-Null
    }

    # Get paths
    $msvcBasePath = Get-MsvcBasePath
    if (-not $msvcBasePath) {
        Write-ColoredOutput "✗ Could not find MSVC installation" $Red
        exit 1
    }
    
    $script:vsPath = "$msvcBasePath\$script:msvcVersion\bin\Hostx64\x64"
    $wdkInclude = Get-WdkIncludePath
    $wdkLib = Get-WdkLibPath
    
    if (-not $wdkInclude -or -not $wdkLib) {
        Write-ColoredOutput "✗ Could not find WDK include or lib paths" $Red
        exit 1
    }

    # Add tools to PATH
    $env:Path = "$script:vsPath;$script:wdkPath;$env:Path"

    Write-ColoredOutput "Compiling driver source files..." $Yellow

    # Compile each source file
    $sourceFiles = @("driver.c", "device.c", "usb.c", "ethernet.c", "ptp.c", "firmware.c")
    $compiledObjects = @()

    foreach ($sourceFile in $sourceFiles) {
        if (-not (Test-Path $sourceFile)) {
            Write-ColoredOutput "⚠️ Source file $sourceFile not found, skipping..." $Yellow
            continue
        }
        
        $objFile = [System.IO.Path]::ChangeExtension($sourceFile, ".obj")
        $compiledObjects += $objFile

        Write-Host "  Compiling $sourceFile..." -ForegroundColor Gray
        
        & cl.exe /c /nologo /W3 /WX- /O2 /Oi /Oy- /D _WIN64 /D _WINDOWS /D NDEBUG `
            /D _NTDDI_WIN10_RS5 /D _WIN32_WINNT=0x0A00 `
            /I"$wdkInclude\km" `
            /I"$wdkInclude\shared" `
            /I"$msvcBasePath\$script:msvcVersion\include" `
            /Fo"$objFile" "$sourceFile"

        if ($LASTEXITCODE -ne 0) {
            Write-ColoredOutput "✗ Compilation of $sourceFile failed" $Red
            exit 1
        }
    }
    
    if ($compiledObjects.Count -eq 0) {
        Write-ColoredOutput "✗ No source files were compiled" $Red
        exit 1
    }

    # Link the driver
    Write-Host "  Linking driver..." -ForegroundColor Gray
    
    & link.exe /nologo /DRIVER /SUBSYSTEM:NATIVE /NODEFAULTLIB `
        /MACHINE:X64 /BASE:0x10000 `
        /OUT:"$BuildDir\$DriverFile" `
        "/LIBPATH:$wdkLib\km\x64" `
        "/LIBPATH:$msvcBasePath\$script:msvcVersion\lib\x64" `
        ntoskrnl.lib hal.lib wdm.lib usbd.lib $compiledObjects

    if ($LASTEXITCODE -eq 0) {
        Write-ColoredOutput "✓ Driver compiled and linked successfully" $Green

        # Copy INF file to build directory if it exists
        if (Test-Path $InfFile) {
            Copy-Item $InfFile "$BuildDir\" -Force
            Write-ColoredOutput "✓ Driver files copied to $BuildDir" $Green
        } else {
            Write-ColoredOutput "⚠️ INF file not found, driver built but not ready for installation" $Yellow
        }
    } else {
        Write-ColoredOutput "✗ Driver linking failed" $Red
        exit 1
    }
}

function Build-Tests {
    Write-ColoredOutput "Building test applications..." $Blue

    if (-not (Test-Path $TestDir)) {
        Write-ColoredOutput "✗ Test directory not found: $TestDir" $Red
        exit 1
    }

    # Get paths
    $msvcBasePath = Get-MsvcBasePath
    $wdkInclude = Get-WdkIncludePath
    $wdkLib = Get-WdkLibPath
    
    if (-not $msvcBasePath -or -not $wdkInclude -or -not $wdkLib) {
        Write-ColoredOutput "✗ Could not find required build tools" $Red
        exit 1
    }

    # Add tools to PATH
    $vsPath = "$msvcBasePath\$script:msvcVersion\bin\Hostx64\x64"
    $env:Path = "$vsPath;$env:Path"

    Write-ColoredOutput "Compiling test applications..." $Yellow

    $testSourceFiles = @("test_framework.c", "hardware_inspection_test.c", "ptp_test.c", "network_test.c", "firmware_test.c")

    foreach ($sourceFile in $testSourceFiles) {
        $testSourcePath = Join-Path $TestDir $sourceFile
        
        if (-not (Test-Path $testSourcePath)) {
            Write-ColoredOutput "⚠️ Test source $sourceFile not found, skipping..." $Yellow
            continue
        }
        
        $exeFile = [System.IO.Path]::ChangeExtension($sourceFile, ".exe")
        $exePath = Join-Path $TestDir $exeFile

        Write-Host "  Compiling $sourceFile..." -ForegroundColor Gray
        
        & cl.exe /nologo /W3 /O2 /D _WIN64 /D _WINDOWS /D _WIN32_WINNT=0x0A00 `
            /I"$wdkInclude\um" `
            /I"$wdkInclude\shared" `
            /I"$msvcBasePath\$script:msvcVersion\include" `
            /Fe"$exePath" "$testSourcePath" `
            /link "/LIBPATH:$wdkLib\um\x64" `
            "/LIBPATH:$msvcBasePath\$script:msvcVersion\lib\x64" `
            setupapi.lib cfgmgr32.lib

        if ($LASTEXITCODE -ne 0) {
            Write-ColoredOutput "✗ Compilation of $sourceFile failed" $Red
            exit 1
        }
    }

    Write-ColoredOutput "✓ Test applications compiled successfully" $Green
}

function Run-Tests {
    Write-ColoredOutput "Running test suite..." $Blue

    if (-not (Test-Path $TestDir)) {
        Write-ColoredOutput "✗ Test directory not found" $Red
        exit 1
    }

    $testExecutables = @(
        @{Name="test_framework.exe"; Description="basic driver tests"},
        @{Name="hardware_inspection_test.exe"; Description="hardware inspection tests"},
        @{Name="ptp_test.exe"; Description="PTP accuracy tests"},
        @{Name="network_test.exe"; Description="network performance tests"},
        @{Name="firmware_test.exe"; Description="firmware programming tests"}
    )

    $allPassed = $true

    foreach ($test in $testExecutables) {
        $testPath = Join-Path $TestDir $test.Name
        
        if (-not (Test-Path $testPath)) {
            Write-ColoredOutput "⚠️ Test executable $($test.Name) not found, skipping..." $Yellow
            continue
        }

        Write-ColoredOutput "Running $($test.Description)..." $Yellow
        & $testPath
        
        if ($LASTEXITCODE -ne 0) {
            Write-ColoredOutput "✗ $($test.Name) failed" $Red
            $allPassed = $false
        } else {
            Write-ColoredOutput "✓ $($test.Name) passed" $Green
        }
    }

    if ($allPassed) {
        Write-ColoredOutput "✓ All tests passed!" $Green
    } else {
        Write-ColoredOutput "✗ Some tests failed" $Red
        exit 1
    }
}

function Install-Driver {
    Write-ColoredOutput "Installing TimeStick driver..." $Blue

    $infPath = Join-Path $BuildDir $InfFile
    
    if (-not (Test-Path $infPath)) {
        Write-ColoredOutput "✗ Driver INF file not found at $infPath" $Red
        Write-ColoredOutput "  Build the driver first with: .\build.ps1 -Build" $Red
        exit 1
    }

    # Use pnputil to install the driver
    try {
        & pnputil.exe /add-driver $infPath /install

        if ($LASTEXITCODE -eq 0) {
            Write-ColoredOutput "✓ Driver installed successfully" $Green
        } else {
            Write-ColoredOutput "✗ Driver installation failed (exit code: $LASTEXITCODE)" $Red
            exit 1
        }
    } catch {
        Write-ColoredOutput "✗ Error running pnputil: $_" $Red
        Write-ColoredOutput "  Please run this script in an elevated PowerShell prompt" $Red
        exit 1
    }
}

function Uninstall-Driver {
    Write-ColoredOutput "Uninstalling TimeStick driver..." $Blue

    try {
        # Get driver OEM INF name
        $driverInfo = & pnputil.exe /enum-drivers 2>&1 | Where-Object { $_ -like "*TimeStick*" }

        if ($driverInfo) {
            # Extract OEM inf name using regex
            $allDriverInfo = & pnputil.exe /enum-drivers 2>&1 | Out-String
            
            if ($allDriverInfo -match "Published Name\s*:\s*(oem\d+\.inf)[\s\S]*?Original Name\s*:\s*TimeStick\.inf") {
                $oemInf = $matches[1]
                
                Write-ColoredOutput "Found driver: $oemInf" $Yellow
                & pnputil.exe /delete-driver $oemInf /uninstall /force

                if ($LASTEXITCODE -eq 0) {
                    Write-ColoredOutput "✓ Driver uninstalled successfully" $Green
                } else {
                    Write-ColoredOutput "✗ Driver uninstall failed (exit code: $LASTEXITCODE)" $Red
                    exit 1
                }
            } else {
                Write-ColoredOutput "✗ Could not parse driver OEM INF name" $Red
                Write-ColoredOutput "  You may need to manually uninstall using Device Manager" $Yellow
            }
        } else {
            Write-ColoredOutput "⚠️ TimeStick driver not found in installed drivers" $Yellow
        }
    } catch {
        Write-ColoredOutput "✗ Error running pnputil: $_" $Red
        exit 1
    }
}

# Main script logic
if ($Help) {
    Show-Help
    exit 0
}

# Check if running as administrator for installation
if (($Install -or $Uninstall)) {
    $currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    if (-not $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        Write-ColoredOutput "✗ Administrator privileges required for driver installation" $Red
        Write-ColoredOutput "  Please run PowerShell as Administrator" $Red
        exit 1
    }
}

# Execute requested operations
if ($Build) {
    Test-Prerequisites
    Build-Driver
    Build-Tests
}

if ($Test) {
    if (-not (Test-Path $TestDir)) {
        Write-ColoredOutput "✗ Test directory not found. Build the project first." $Red
        exit 1
    }
    Run-Tests
}

if ($Install) {
    Install-Driver
}

if ($Uninstall) {
    Uninstall-Driver
}

# Default action if no options specified
if (-not ($Build -or $Test -or $Install -or $Uninstall -or $Help)) {
    Write-ColoredOutput "No action specified. Use -Help for usage information." $Yellow
    Show-Help
    exit 1
}

Write-ColoredOutput "`nScript completed successfully!" $Green
