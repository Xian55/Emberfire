# Deploy the freshly built client into run/bin and launch it.
# The client does SetCurrentDirectory("..") at startup, so the exe lives in run/bin and the assets
# (config.ini, autoexec.conf, game.db, content/maps/scripts/cache) live in run/. The asset junctions
# point at the local game data (override -GameDir if it lives elsewhere). run/ is gitignored.
param(
    [string]$Config  = 'Release',
    # Game runtime (content/maps/scripts/game.db). Defaults to the EMBERFIRE_GAME_DIR env var.
    [string]$GameDir = $env:EMBERFIRE_GAME_DIR
)
$ErrorActionPreference = 'Stop'
if (-not $GameDir) { throw "Set EMBERFIRE_GAME_DIR (setx EMBERFIRE_GAME_DIR '<game runtime dir>') or pass -GameDir." }
$root = Split-Path -Parent $PSScriptRoot
$built = Join-Path $root "build\x86-$($Config.ToLower())\client\$Config"
$run   = Join-Path $root 'run'
$bin   = Join-Path $run 'bin'

if (-not (Test-Path (Join-Path $built 'emberfire_client.exe'))) {
    throw "Build not found: $built\emberfire_client.exe. Build first:  cmake --build --preset x86-$($Config.ToLower())"
}
New-Item -ItemType Directory -Force -Path $bin | Out-Null

# Assets in the cd-target (run/) + junctions to the game data.
foreach ($f in 'config.ini','autoexec.conf','game.db') { Copy-Item (Join-Path $GameDir $f) $run -Force }
foreach ($d in 'content','maps','scripts','cache') {
    $link = Join-Path $run $d
    if (-not (Test-Path $link)) { New-Item -ItemType Junction -Path $link -Target (Join-Path $GameDir $d) | Out-Null }
}

# Built-in default UI (client/ui) -> run/ui as a JUNCTION so the client (cwd = run/) finds it and loads the
# Lua UI instead of falling back to the C++ HUD. Without this the Lua UI is silently skipped. Live for /reload.
$uiSrc = Join-Path $root 'client\ui'
$uiDst = Join-Path $run 'ui'
if (Test-Path $uiSrc) {
    $existing = Get-Item $uiDst -ErrorAction SilentlyContinue
    if ($existing -and $existing.LinkType -ne 'Junction') { Remove-Item $uiDst -Recurse -Force }
    if (-not (Test-Path $uiDst)) { New-Item -ItemType Junction -Path $uiDst -Target $uiSrc | Out-Null }
}

# In-repo UI art (client/ui/art) -> run/content-ember as a JUNCTION. ContentMgr scans this overlay for zips
# and getTexture() falls back to loose PNGs here, so the 9-slice/chrome art loads without touching the
# pristine game content/ (a junction to the external game dir). Live for /reload.
$artSrc = Join-Path $root 'client\ui\art'
$artDst = Join-Path $run 'content-ember'
if (Test-Path $artSrc) {
    $existingArt = Get-Item $artDst -ErrorAction SilentlyContinue
    if ($existingArt -and $existingArt.LinkType -ne 'Junction') { Remove-Item $artDst -Recurse -Force }
    if (-not (Test-Path $artDst)) { New-Item -ItemType Junction -Path $artDst -Target $artSrc | Out-Null }
}

# Bundled example addons (client/addons) -> run/addons. Seed each only if missing, so player edits/removals
# and their own addons are never clobbered. The client loads addons/ from run/ (cwd).
$addonSrc = Join-Path $root 'client\addons'
if (Test-Path $addonSrc) {
    $addonDst = Join-Path $run 'addons'
    if (-not (Test-Path $addonDst)) { New-Item -ItemType Directory -Force $addonDst | Out-Null }
    foreach ($a in (Get-ChildItem $addonSrc -Directory)) {
        if (-not (Test-Path (Join-Path $addonDst $a.Name))) { Copy-Item $a.FullName $addonDst -Recurse -Force }
    }
}

# Exe + DLLs (+ config/db copies, mirroring the working out/Release layout).
Copy-Item (Join-Path $built 'emberfire_client.exe') $bin -Force
Copy-Item (Join-Path $built '*.dll') $bin -Force
foreach ($f in 'config.ini','game.db') { Copy-Item (Join-Path $GameDir $f) $bin -Force }

Write-Host "Launching $bin\emberfire_client.exe"
Start-Process -FilePath (Join-Path $bin 'emberfire_client.exe') -WorkingDirectory $bin
