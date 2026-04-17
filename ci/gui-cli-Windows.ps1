Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"
if ($PSVersionTable.PSVersion.Major -ge 7) {
  $PSNativeCommandUseErrorActionPreference = $true
}

function New-Directory {
  param([Parameter(Mandatory)] [string]$Path)
  New-Item -ItemType Directory -Force -Path $Path | Out-Null
}

function Set-DefaultEnvironmentValue {
  param(
    [Parameter(Mandatory)] [string]$Name,
    [Parameter(Mandatory)] [string]$Value
  )

  if (-not [System.Environment]::GetEnvironmentVariable($Name)) {
    Set-Item -Path "Env:$Name" -Value $Value
  }
}

function Resolve-LibraryPath {
  param(
    [Parameter(Mandatory)] [string[]]$Patterns,
    [string]$Root = (Join-Path $env:INSTALL_PREFIX "lib")
  )

  foreach ($pattern in $Patterns) {
    $match = Get-ChildItem -Path (Join-Path $Root $pattern) -File -ErrorAction SilentlyContinue |
      Sort-Object FullName |
      Select-Object -First 1
    if ($match) {
      return $match.FullName
    }
  }

  throw "Could not find library matching any of: $($Patterns -join ', ') under $Root"
}

function Invoke-LoggedCommand {
  param(
    [Parameter(Mandatory)] [string]$FilePath,
    [string[]]$ArgumentList = @()
  )

  Write-Host "> $FilePath $($ArgumentList -join ' ')"
  & $FilePath @ArgumentList
  if ($LASTEXITCODE -ne 0) {
    throw "Command failed with exit code ${LASTEXITCODE}: $FilePath $($ArgumentList -join ' ')"
  }
}

$requiredEnvVars = @("INSTALL_PREFIX", "PYTHON_EXE")
foreach ($name in $requiredEnvVars) {
  if (-not (Get-Item -Path "Env:$name" -ErrorAction SilentlyContinue)) {
    throw "$name is not set"
  }
}

Set-DefaultEnvironmentValue -Name "CMAKE_POLICY_VERSION_MINIMUM" -Value "3.5"
Set-DefaultEnvironmentValue -Name "CMAKE_GENERATOR" -Value "Ninja"
Set-DefaultEnvironmentValue -Name "CMAKE_C_COMPILER" -Value "cl"
Set-DefaultEnvironmentValue -Name "CMAKE_CXX_COMPILER" -Value "cl"
Set-DefaultEnvironmentValue -Name "CMAKE_MSVC_RUNTIME_LIBRARY" -Value 'MultiThreaded$<$<CONFIG:Debug>:Debug>'

$pythonExe = (Get-Command $env:PYTHON_EXE -ErrorAction Stop).Source
$prefixPath = "$($env:INSTALL_PREFIX);$($env:INSTALL_PREFIX)\CMake;$($env:INSTALL_PREFIX)\lib\cmake"
$cudaRoot = Join-Path $env:INSTALL_PREFIX "cuda"
$cudaBinDir = Join-Path $cudaRoot "bin"
if (Test-Path -LiteralPath $cudaBinDir) {
  $env:PATH = "$cudaBinDir;$env:PATH"
}
$freetypeLib = Resolve-LibraryPath @("CombinedFreetype.lib", "*BundledFreetype*.lib")
$expatLib = Resolve-LibraryPath @("libexpatMT*.lib", "libexpatMD*.lib", "libexpat*.lib")
$qtFreetypeIncludeDir = Join-Path $env:INSTALL_PREFIX "include\QtFreetype"

Write-Host "INSTALL_PREFIX = $env:INSTALL_PREFIX"
Write-Host "RUNNER_ARCH = $env:RUNNER_ARCH"
Write-Host "CMAKE_POLICY_VERSION_MINIMUM = $env:CMAKE_POLICY_VERSION_MINIMUM"
Write-Host "CMAKE_GENERATOR = $env:CMAKE_GENERATOR"
Write-Host "CMAKE_C_COMPILER = $env:CMAKE_C_COMPILER"
Write-Host "CMAKE_CXX_COMPILER = $env:CMAKE_CXX_COMPILER"
Write-Host "CMAKE_MSVC_RUNTIME_LIBRARY = $env:CMAKE_MSVC_RUNTIME_LIBRARY"
Write-Host "CMAKE_CXX_FLAGS = $env:CMAKE_CXX_FLAGS"
Write-Host "PYTHON_EXE = $pythonExe"
Write-Host "PATH = $env:PATH"
Write-Host "git = $((Get-Command git -ErrorAction Stop).Source)"
Invoke-LoggedCommand git @("--version")
Write-Host "cl = $((Get-Command cl -ErrorAction Stop).Source)"
Write-Host "ninja = $((Get-Command ninja -ErrorAction Stop).Source)"
Invoke-LoggedCommand ninja @("--version")
Write-Host "cmake = $((Get-Command cmake -ErrorAction Stop).Source)"
Invoke-LoggedCommand cmake @("--version")
Write-Host "python = $pythonExe"
Invoke-LoggedCommand $pythonExe @("--version")
if (Get-Command ccache -ErrorAction SilentlyContinue) {
  Invoke-LoggedCommand ccache @("--version")
}
if (Get-Command dumpbin -ErrorAction SilentlyContinue) {
  Write-Host "dumpbin = $((Get-Command dumpbin -ErrorAction Stop).Source)"
}

if (Test-Path -LiteralPath "build") {
  Remove-Item -LiteralPath "build" -Recurse -Force
}
New-Directory "build"
Push-Location "build"
try {
  $cmakeArgs = @(
    "..",
    "-G$env:CMAKE_GENERATOR",
    "-DCMAKE_BUILD_TYPE=Release",
    "-DCMAKE_TRY_COMPILE_CONFIGURATION=Release",
    "-DCMAKE_C_COMPILER=$env:CMAKE_C_COMPILER",
    "-DCMAKE_CXX_COMPILER=$env:CMAKE_CXX_COMPILER",
    "-DCMAKE_CXX_FLAGS=$env:CMAKE_CXX_FLAGS",
    "-DCMAKE_MSVC_RUNTIME_LIBRARY=$env:CMAKE_MSVC_RUNTIME_LIBRARY",
    "-DCMAKE_PREFIX_PATH=$prefixPath",
    "-DCUDAToolkit_ROOT=$cudaRoot",
    "-DSME_LOG_LEVEL=OFF",
    "-DEXPAT_INCLUDE_DIR=$(Join-Path $env:INSTALL_PREFIX 'include')",
    "-DEXPAT_LIBRARY=$expatLib",
    "-DFREETYPE_LIBRARY_RELEASE=$freetypeLib",
    "-DFREETYPE_INCLUDE_DIR_freetype2=$qtFreetypeIncludeDir",
    "-DFREETYPE_INCLUDE_DIR_ft2build=$qtFreetypeIncludeDir",
    "-DPython_EXECUTABLE=$pythonExe"
  )
  if (Get-Command ccache -ErrorAction SilentlyContinue) {
    $cmakeArgs += @(
      "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
      "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    )
  }

  Invoke-LoggedCommand cmake $cmakeArgs
  Invoke-LoggedCommand cmake @("--build", ".", "--parallel", "--verbose")

  if (Get-Command ccache -ErrorAction SilentlyContinue) {
    Invoke-LoggedCommand ccache @("--show-stats")
  }

  $pythonModule = Get-ChildItem -Path ".\sme" -Filter "*.pyd" -File |
    Sort-Object FullName |
    Select-Object -First 1
  if (-not $pythonModule) {
    throw "Could not find built Python extension under build\sme"
  }

  if (Get-Command dumpbin -ErrorAction SilentlyContinue) {
    Invoke-LoggedCommand dumpbin @("/NOLOGO", "/DEPENDENTS", $pythonModule.FullName)
  }

  $testsOutput = "tests.txt"
  try {
    & ".\test\tests.exe" -as "~[gui]~[requires-gpu]~[requires-cuda-gpu]" *> $testsOutput
    if ($LASTEXITCODE -ne 0) {
      throw "tests.exe exited with code $LASTEXITCODE"
    }
  } catch {
    if (Test-Path -LiteralPath $testsOutput) {
      Get-Content -Path $testsOutput -Tail 1000
    }
    throw
  }
  Get-Content -Path $testsOutput -Tail 100

  Invoke-LoggedCommand ".\benchmark\benchmark.exe" @("1")

  Push-Location ".\sme"
  try {
    Invoke-LoggedCommand $pythonExe @("-m", "pip", "install", "-r", "..\..\sme\requirements-test.txt")
    Invoke-LoggedCommand $pythonExe @("-m", "pytest", "..\..\sme\test", "-v")

    $previousPyPath = [System.Environment]::GetEnvironmentVariable("PYTHONPATH")
    $previousMplBackend = [System.Environment]::GetEnvironmentVariable("MPLBACKEND")
    $previousPyFaulthandler = [System.Environment]::GetEnvironmentVariable("PYTHONFAULTHANDLER")
    $env:PYTHONPATH = (Get-Location).Path
    $env:MPLBACKEND = "Agg"
    $env:PYTHONFAULTHANDLER = "1"
    try {
      Invoke-LoggedCommand $pythonExe @("-X", "faulthandler", "..\..\sme\test\sme_doctest.py", "-v")
    } finally {
      if ($null -eq $previousPyPath) {
        Remove-Item Env:PYTHONPATH -ErrorAction SilentlyContinue
      } else {
        $env:PYTHONPATH = $previousPyPath
      }
      if ($null -eq $previousMplBackend) {
        Remove-Item Env:MPLBACKEND -ErrorAction SilentlyContinue
      } else {
        $env:MPLBACKEND = $previousMplBackend
      }
      if ($null -eq $previousPyFaulthandler) {
        Remove-Item Env:PYTHONFAULTHANDLER -ErrorAction SilentlyContinue
      } else {
        $env:PYTHONFAULTHANDLER = $previousPyFaulthandler
      }
    }
  } finally {
    Pop-Location
  }

  if (Get-Command dumpbin -ErrorAction SilentlyContinue) {
    Invoke-LoggedCommand dumpbin @("/NOLOGO", "/DEPENDENTS", ".\app\spatial-model-editor.exe")
    Invoke-LoggedCommand dumpbin @("/NOLOGO", "/DEPENDENTS", ".\cli\spatial-cli.exe")
  }

  Invoke-LoggedCommand ".\app\spatial-model-editor.exe" @("-v")
} finally {
  Pop-Location
}

if (Get-Command ccache -ErrorAction SilentlyContinue) {
  Invoke-LoggedCommand ccache @("--show-stats")
}

New-Directory "artifacts\binaries"
$guiArtifactName = "spatial-model-editor.exe"
$cliArtifactName = "spatial-cli.exe"
if ($env:RUNNER_ARCH -eq "ARM64") {
  $guiArtifactName = "spatial-model-editor-ARM64.exe"
  $cliArtifactName = "spatial-cli-ARM64.exe"
}
Move-Item -Path "build\app\spatial-model-editor.exe" -Destination ("artifacts\binaries\" + $guiArtifactName) -Force
Move-Item -Path "build\cli\spatial-cli.exe" -Destination ("artifacts\binaries\" + $cliArtifactName) -Force
