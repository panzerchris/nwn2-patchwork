param(
    [string]$BuildDir = 'build',
    [string]$Configuration = 'Release',
    [string]$OutputDir = 'dist'
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$dirty = & git -C $root status --porcelain
if ($LASTEXITCODE -ne 0 -or $dirty) { throw 'Package from a clean, committed Git checkout.' }
$build = [IO.Path]::GetFullPath((Join-Path $root $BuildDir))
$output = [IO.Path]::GetFullPath((Join-Path $root $OutputDir))
$cmakeText = Get-Content -LiteralPath (Join-Path $root 'CMakeLists.txt') -Raw
if ($cmakeText -notmatch 'project\(nwn2_patchwork VERSION ([0-9]+\.[0-9]+\.[0-9]+)') {
    throw 'Cannot determine project version.'
}
$version = $Matches[1]
if ($env:GITHUB_REF_TYPE -eq 'tag' -and $env:GITHUB_REF_NAME -ne "v$version") {
    throw "Tag $($env:GITHUB_REF_NAME) does not match project v$version."
}
# Never remove a previous package or stage. Choose a new output folder to rebuild.
$stage = Join-Path $output 'stage'
if (Test-Path -LiteralPath $stage) { throw "Stage already exists: $stage. Choose a new OutputDir." }
$package = $stage
& cmake --install $build --config $Configuration --prefix $package
if ($LASTEXITCODE -ne 0) { throw 'CMake install failed.' }
$expected = @('nwn2-patchwork.ini',
    'README.md','LICENSE','CHANGELOG.md','CONTRIBUTING.md','docs/compatibility.md','docs/configuration.md',
    'docs/development.md','docs/troubleshooting.md','docs/validation.md','docs/release-notes.md','docs/loading.md',
    'docs/multiplayer.md')
# Include small, committed release validation records if present.
$records = Join-Path $root 'docs/releases'
if (Test-Path -LiteralPath $records) {
    $expected += @(Get-ChildItem -LiteralPath $records -File | ForEach-Object { "docs/releases/$($_.Name)" })
}
$expected = @('dinput8.dll') + @($expected | ForEach-Object { "nwn2-patchwork/$_" })
$files = @(Get-ChildItem -LiteralPath $package -Recurse -File | ForEach-Object {
    $relative = $_.FullName.Substring($package.Length+1).Replace('\','/')
    if ($relative -notin $expected) { throw "Unexpected release file: $relative" }
    [ordered]@{path=$relative; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
})
foreach ($name in $expected) {
    if ($name -notin $files.path) { throw "Missing release file: $name" }
}
$commit = (& git -C $root rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Package from a Git checkout with a recorded commit.' }
$manifest = [ordered]@{project='NWN2 Patchwork';version=$version;commit=$commit;
    configuration=$Configuration;architecture='x86';files=$files}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $package 'nwn2-patchwork/build-manifest.json') -Encoding utf8
$archive = Join-Path $output "nwn2-patchwork-$version-windows-x86.zip"
if (Test-Path -LiteralPath $archive) { throw 'Archive already exists; choose a new output folder.' }
Compress-Archive -LiteralPath @((Join-Path $package 'dinput8.dll'),(Join-Path $package 'nwn2-patchwork')) -DestinationPath $archive
$hash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()
"$hash  $([IO.Path]::GetFileName($archive))" | Set-Content -LiteralPath "$archive.sha256" -Encoding ascii
Write-Output "Created $archive"
Write-Output "SHA-256 $hash"
