# Private owner-source build only. Does not upload source or artifacts.
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$JucePath,
    [Parameter(Mandatory=$true)][string]$NativeSourceRoot,
    [string]$BuildDir = 'build-owner-windows',
    [string]$PluginvalPath,
    [ValidateRange(1,16)][int]$Jobs = 2
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:OS -ne 'Windows_NT') { throw 'Run this in Windows PowerShell with the Visual Studio C++ toolchain installed.' }
function Invoke-Checked([string]$Exe, [string[]]$Arguments) {
    & $Exe @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Exe failed ($LASTEXITCODE): $($Arguments -join ' ')" }
}
$Repo = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Juce = (Resolve-Path $JucePath).Path
$Native = (Resolve-Path $NativeSourceRoot).Path
$Build = [IO.Path]::GetFullPath((Join-Path $Repo $BuildDir))
if ([IO.Path]::IsPathRooted($BuildDir)) { $Build = [IO.Path]::GetFullPath($BuildDir) }
if (-not (Test-Path (Join-Path $Juce 'CMakeLists.txt'))) { throw 'JUCE_PATH must point to extracted JUCE 8.0.12.' }
if ($PluginvalPath) { $PluginvalPath = (Resolve-Path $PluginvalPath).Path }
Push-Location $Repo
try {
    Invoke-Checked cmake @('-S',$Repo,'-B',$Build,"-DJUCE_PATH=$Juce",'-DFETCHCONTENT_FULLY_DISCONNECTED=ON','-DCMAKE_BUILD_TYPE=Release','-DSKELLIERE_BUILD_UI_RENDERER=ON','-DSKELLIERE_NATIVE_IMPORTS=ON',"-DSKELLIERE_NATIVE_SOURCE_ROOT=$Native")
    Invoke-Checked cmake @('--build',$Build,'--config','Release','--target','SkellierRack_VST3','SkelliereRackUIRenderer','--parallel',"$Jobs")
    Invoke-Checked python @('tools/fix_vst3_moduleinfo.py',$Build)
    $Bundle = Get-ChildItem (Join-Path $Build 'SkellierRack_artefacts') -Recurse -Directory -Filter '*.vst3' | Where-Object { $_.Parent.Name -eq 'VST3' } | Select-Object -First 1
    if (-not $Bundle) { throw 'VST3 bundle not found' }
    $Binary = Get-ChildItem $Bundle.FullName -Recurse -File -Filter '*.vst3' | Where-Object { $_.Directory.Name -eq 'x86_64-win' } | Select-Object -First 1
    if (-not $Binary) { throw 'Windows x86_64 VST3 binary not found' }
    $Renderer = Get-ChildItem $Build -Recurse -File -Filter 'SkelliereRackUIRenderer.exe' | Select-Object -First 1
    if (-not $Renderer) { throw 'Native renderer not found' }
    $CaptureDir = Join-Path $Build 'owner-ui-evidence'
    Invoke-Checked $Renderer.FullName @('--consolidated','--output',$CaptureDir)
    foreach ($Name in @('native-delay.png','native-glitch.png')) { if (-not (Test-Path (Join-Path $CaptureDir $Name))) { throw "Missing $Name" } }
    $PluginvalStatus = 'NOT RUN — test build only'
    if ($PluginvalPath) {
        Invoke-Checked $PluginvalPath @('--strictness-level','5','--validate',$Bundle.FullName)
        $PluginvalStatus = 'PASSED — strictness 5'
    }
    $Output = Join-Path $Build 'owner-test-package'
    if (Test-Path $Output) { Remove-Item $Output -Recurse -Force }
    New-Item -ItemType Directory $Output | Out-Null
    Copy-Item $Bundle.FullName $Output -Recurse
    Copy-Item $CaptureDir (Join-Path $Output 'ui-evidence') -Recurse
    $Hash = (Get-FileHash -Algorithm SHA256 $Binary.FullName).Hash
    $Commit = (& git rev-parse HEAD 2>$null | Out-String).Trim()
    if ($LASTEXITCODE -ne 0) { $Commit = 'Source archive / inspect source manifest' }
    @("Source HEAD: $Commit", "Binary SHA256: $Hash", 'Owner-source Windows test build; not a public release.', "Pluginval: $PluginvalStatus", 'FL Studio manual validation: NOT RUN') | Set-Content (Join-Path $Output 'BUILD_EVIDENCE.txt')
    $Archive = Join-Path $Build 'Skelliere-34-Effects-OWNER-TEST.zip'
    Compress-Archive -Path (Join-Path $Output '*') -DestinationPath $Archive -Force
    Write-Host "Owner test archive: $Archive"
} finally { Pop-Location }
