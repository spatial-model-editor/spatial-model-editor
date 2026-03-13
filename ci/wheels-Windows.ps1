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

function Convert-ToCMakePath {
  param([Parameter(Mandatory)] [string]$Path)

  return ($Path -replace "\\", "/")
}

function Format-CMakeArg {
  param([Parameter(Mandatory)] [string]$Argument)

  if ($Argument.Contains(" ")) {
    return '"' + $Argument + '"'
  }
  return $Argument
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

$requiredEnvVars = @("INSTALL_PREFIX", "PYTHON_EXE", "CIBUILDWHEEL_VERSION")
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
$installPrefixCMake = Convert-ToCMakePath $env:INSTALL_PREFIX
$prefixPath = "$($env:INSTALL_PREFIX);$($env:INSTALL_PREFIX)\CMake;$($env:INSTALL_PREFIX)\lib\cmake"
$prefixPathCMake = "$installPrefixCMake;$installPrefixCMake/CMake;$installPrefixCMake/lib/cmake"
$cudaRoot = Join-Path $env:INSTALL_PREFIX "cuda"
$cudaRootCMake = "$installPrefixCMake/cuda"
$cudaBinDir = Join-Path $cudaRoot "bin"
if (Test-Path -LiteralPath $cudaBinDir) {
  $env:PATH = "$cudaBinDir;$env:PATH"
}
$freetypeLib = Resolve-LibraryPath @("CombinedFreetype.lib", "*BundledFreetype*.lib")
$freetypeLibCMake = Convert-ToCMakePath $freetypeLib
$expatLib = Resolve-LibraryPath @("libexpatMT*.lib", "libexpatMD*.lib", "libexpat*.lib")
$expatLibCMake = Convert-ToCMakePath $expatLib
$expatIncludeDirCMake = "$installPrefixCMake/include"
$qtFreetypeIncludeDir = Join-Path $env:INSTALL_PREFIX "include\QtFreetype"
$qtFreetypeIncludeDirCMake = "$installPrefixCMake/include/QtFreetype"

Write-Host "INSTALL_PREFIX = $env:INSTALL_PREFIX"
Write-Host "CIBUILDWHEEL_VERSION = $env:CIBUILDWHEEL_VERSION"
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
  Invoke-LoggedCommand ccache @("--show-stats")
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
    "-DCMAKE_INSTALL_PREFIX=$installPrefixCMake",
    "-DBUILD_TESTING=ON",
    "-DSME_BUILD_BENCHMARKS=OFF",
    "-DSME_BUILD_CLI=OFF",
    "-DSME_BUILD_GUI=OFF",
    "-DSME_BUILD_PYTHON_LIBRARY=OFF",
    "-DSME_BUILD_CORE=ON",
    "-DSME_LOG_LEVEL=OFF",
    "-DCMAKE_PREFIX_PATH=$prefixPathCMake",
    "-DCUDAToolkit_ROOT=$cudaRootCMake",
    "-DCMAKE_CXX_VISIBILITY_PRESET=hidden",
    "-DEXPAT_INCLUDE_DIR=$expatIncludeDirCMake",
    "-DEXPAT_LIBRARY=$expatLibCMake",
    "-DFREETYPE_LIBRARY_RELEASE=$freetypeLibCMake",
    "-DFREETYPE_INCLUDE_DIR_freetype2=$qtFreetypeIncludeDirCMake",
    "-DFREETYPE_INCLUDE_DIR_ft2build=$qtFreetypeIncludeDirCMake"
  )
  if (Get-Command ccache -ErrorAction SilentlyContinue) {
    $cmakeArgs += @(
      "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
      "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
    )
  }

  Invoke-LoggedCommand cmake $cmakeArgs
  Invoke-LoggedCommand cmake @("--build", ".", "--parallel", "--target", "core", "tests", "--verbose")
  $testsOutput = "tests.txt"
  try {
    & ".\test\tests.exe" -as "~[requires-cuda-gpu]" *> $testsOutput
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
  Invoke-LoggedCommand cmake @("--install", ".")
} finally {
  Pop-Location
}

if (Get-Command ccache -ErrorAction SilentlyContinue) {
  Invoke-LoggedCommand ccache @("--show-stats")
}

$cibwCMakeArgs = @(
  "-DSME_LOG_LEVEL=OFF",
  "-DSME_BUILD_CORE=OFF",
  "-DCMAKE_TRY_COMPILE_CONFIGURATION=Release",
  "-DCMAKE_MSVC_RUNTIME_LIBRARY=$env:CMAKE_MSVC_RUNTIME_LIBRARY",
  "-DCMAKE_CXX_FLAGS=$env:CMAKE_CXX_FLAGS",
  "-DCMAKE_PREFIX_PATH=$prefixPathCMake",
  "-DCUDAToolkit_ROOT=$cudaRootCMake",
  "-DCMAKE_CXX_VISIBILITY_PRESET=hidden",
  "-DEXPAT_INCLUDE_DIR=$expatIncludeDirCMake",
  "-DEXPAT_LIBRARY=$expatLibCMake",
  "-DFREETYPE_LIBRARY_RELEASE=$freetypeLibCMake",
  "-DFREETYPE_INCLUDE_DIR_freetype2=$qtFreetypeIncludeDirCMake",
  "-DFREETYPE_INCLUDE_DIR_ft2build=$qtFreetypeIncludeDirCMake"
)
if (Get-Command ccache -ErrorAction SilentlyContinue) {
  $cibwCMakeArgs += @(
    "-DCMAKE_C_COMPILER_LAUNCHER=ccache",
    "-DCMAKE_CXX_COMPILER_LAUNCHER=ccache"
  )
}

$env:CMAKE_PREFIX_PATH = $prefixPathCMake
$env:CUDAToolkit_ROOT = $cudaRootCMake
$env:CMAKE_ARGS = ($cibwCMakeArgs | ForEach-Object { Format-CMakeArg $_ }) -join " "

Invoke-LoggedCommand $pythonExe @("-m", "pip", "install", "cibuildwheel==$env:CIBUILDWHEEL_VERSION")
Invoke-LoggedCommand $pythonExe @("-m", "cibuildwheel", "--output-dir", ".\artifacts\dist")
