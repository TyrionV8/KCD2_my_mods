param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# This script only writes below this project directory. It does not edit the game.
$project = $PSScriptRoot
$work = Join-Path $project '_build'
$lib = Join-Path $work 'libKCD2'
$kcse = Join-Path $work 'KCSE-for-kcd2'
$vcpkg = Join-Path $work 'vcpkg'
$build = Join-Path $work 'cmake'
$bundle = Join-Path $project 'dist\KingdomComeDeliverance2'

function Run([string]$program, [string[]]$arguments) {
    & $program @arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$program failed with exit code $LASTEXITCODE"
    }
}

function Clone-Pinned([string]$url, [string]$commit, [string]$destination) {
    if (-not (Test-Path $destination)) {
        Run 'git' @('clone', '--depth', '1', $url, $destination)
    }
    $origin = (& git -C $destination remote get-url origin).Trim()
    if ($LASTEXITCODE -ne 0 -or $origin -ne $url) {
        throw "Unexpected Git repository at $destination"
    }
    $current = (& git -C $destination rev-parse HEAD).Trim()
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect $destination" }
    if ($current -ne $commit) {
        $changes = & git -C $destination status --porcelain
        if ($LASTEXITCODE -ne 0 -or $changes) {
            throw "Local changes in $destination; refusing to change its revision"
        }
        Run 'git' @('-C', $destination, 'fetch', '--depth', '1', 'origin', $commit)
        Run 'git' @('-C', $destination, 'checkout', '--detach', 'FETCH_HEAD')
    }
}

foreach ($command in @('git', 'cmake')) {
    if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
        throw "Install $command and add it to PATH before running this script"
    }
}
New-Item -ItemType Directory -Force -Path $work | Out-Null

Clone-Pinned 'https://github.com/JerryYOJ/libKCD2.git' `
    '10d20f28faba462c4bf98a01abb48225cc51bb91' $lib
Clone-Pinned 'https://github.com/JerryYOJ/KCSE-for-kcd2.git' `
    '47ff1bd75c4df4a3a2145e7ba9908d5b2e7f7aec' $kcse
if (-not (Test-Path $vcpkg)) {
    Run 'git' @('clone', '--depth', '1', 'https://github.com/microsoft/vcpkg.git', $vcpkg)
}
if (-not (Test-Path (Join-Path $vcpkg 'vcpkg.exe'))) {
    Run (Join-Path $vcpkg 'bootstrap-vcpkg.bat') @('-disableMetrics')
}

$pluginProject = Join-Path $lib 'Projects\InstantPrepare'
$kcseProject = Join-Path $lib 'Projects\KCSE'
New-Item -ItemType Directory -Force -Path `
    (Join-Path $pluginProject 'src'), (Join-Path $pluginProject '.buildenv'), `
    (Join-Path $kcseProject 'src'), (Join-Path $kcseProject '.buildenv') | Out-Null
Copy-Item -Recurse -Force (Join-Path $project 'src\*') (Join-Path $pluginProject 'src')
Copy-Item -Recurse -Force (Join-Path $project '.buildenv\*') (Join-Path $pluginProject '.buildenv')
Copy-Item -Recurse -Force (Join-Path $kcse 'src\*') (Join-Path $kcseProject 'src')
Copy-Item -Recurse -Force (Join-Path $kcse '.buildenv\*') (Join-Path $kcseProject '.buildenv')

$env:VCPKG_ROOT = $vcpkg
$toolchain = Join-Path $vcpkg 'scripts\buildsystems\vcpkg.cmake'
Run 'cmake' @('-S', (Join-Path $lib '.buildenv'), '-B', $build,
    '-G', 'Visual Studio 17 2022', '-A', 'x64',
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain", '-DVCPKG_TARGET_TRIPLET=x64-windows-static-md')
Run 'cmake' @('--build', $build, '--config', 'Release',
    '--target', 'KCSE', 'InstantPrepare', '--parallel', '4')

$pluginDll = Join-Path $build 'InstantPrepare\Release\InstantPrepare.dll'
$kcseDll = Join-Path $build 'KCSE\Release\dinput8.dll'
if (-not (Test-Path $pluginDll) -or -not (Test-Path $kcseDll)) {
    throw 'Build finished without the expected DLL files'
}

$modFolder = Join-Path $bundle 'Mods\InstantPrepare'
$binFolder = Join-Path $bundle 'Bin\Win64MasterMasterSteamPGO'
$addressFolder = Join-Path $bundle 'KCSE\addresslib'
$pluginFolder = Join-Path $modFolder 'KCSE\Plugins'
New-Item -ItemType Directory -Force -Path `
    $modFolder, $binFolder, $addressFolder, $pluginFolder | Out-Null

Copy-Item -Force $pluginDll (Join-Path $pluginFolder 'InstantPrepare.dll')
Copy-Item -Force $kcseDll (Join-Path $binFolder 'dinput8.dll')
Copy-Item -Force (Join-Path $project 'mod.manifest') $modFolder
Copy-Item -Force (Join-Path $project 'LICENSE') $modFolder

$addressName = 'kcd_addresslib_steam_release_1_5-15693.bin'
$addressFile = Join-Path $addressFolder $addressName
$addressHash = '9a992aa21d9046c1f1c67b8fbc54a51f63adea2c5ba692d7b15e6bfc4760f06c'
if (-not (Test-Path $addressFile) -or
    (Get-FileHash $addressFile -Algorithm SHA256).Hash.ToLowerInvariant() -ne $addressHash) {
    $uri = 'https://raw.githubusercontent.com/JerryYOJ/Address-Library-For-KCSE/' + `
        '0294d829acc68867c8df2e6bbc5770e415161cc4/kcd2_address_library/' + $addressName
    $download = "$addressFile.download"
    Invoke-WebRequest -Uri $uri -OutFile $download
    if ((Get-FileHash $download -Algorithm SHA256).Hash.ToLowerInvariant() -ne $addressHash) {
        throw 'Address library hash mismatch; package was not completed'
    }
    Move-Item -Force $download $addressFile
}

$archive = Join-Path $project 'dist\InstantPrepare-1.5.6-steam.zip'
Compress-Archive -Path $bundle -DestinationPath $archive -Force
Write-Host "Package ready: $archive"
Write-Host "Plugin: $pluginDll"
Write-Host "KCSE loader: $kcseDll"
Write-Host "Address library SHA256: $addressHash"
