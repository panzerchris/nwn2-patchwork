param([string]$Launcher,[string]$Fixture,[string]$Config)
$ErrorActionPreference='Stop'
$temporary=Join-Path ([IO.Path]::GetTempPath()) ('patchwork-test-'+[Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporary | Out-Null
try {
    $fakeGame=Join-Path $temporary 'nwn2main.exe'
    Copy-Item -LiteralPath $Fixture -Destination $fakeGame
    $before=[Convert]::ToBase64String([IO.File]::ReadAllBytes($fakeGame))
    function Expect-Refusal([string[]]$Arguments,[string]$Message) {
        # PS5 treats native stderr as ErrorRecord; retain output before checking exit.
        $saved=$ErrorActionPreference
        try {$ErrorActionPreference='Continue';$result=& $Launcher @Arguments 2>&1;$code=$LASTEXITCODE}
        finally {$ErrorActionPreference=$saved}
        if($code -ne 1 -or ($result | Out-String) -notmatch $Message) {
            throw "Wrong refusal: exit=$code; output=$result"
        }
    }
    Expect-Refusal @('--not-an-option') 'Unknown or incomplete option'
    Expect-Refusal @('--game',$Fixture,'--config',$Config,'--check') 'installed nwn2main.exe'
    Expect-Refusal @('--game',$fakeGame,'--config',$Config,'--check') 'Unsupported nwn2main.exe SHA-256'
    $badIni=Join-Path $temporary 'invalid.ini'
    '[Display]', 'BorderlessAltEnter=1', 'NativeSizing=0', 'FitWindowedApply=0' |
        Set-Content -LiteralPath $badIni -Encoding ascii
    Expect-Refusal @('--game',$fakeGame,'--config',$badIni,'--check') 'BorderlessAltEnter=1 requires NativeSizing=1'
    if([Convert]::ToBase64String([IO.File]::ReadAllBytes($fakeGame)) -ne $before) {throw 'Fixture changed'}
    Write-Output 'Launcher refused unknown arguments, renamed/unsupported executables and invalid dependencies with the expected messages.'
} finally {
    # Only this newly created, verified temp directory may be removed.
    $resolved=[IO.Path]::GetFullPath($temporary)
    $tempRoot=[IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\')+'\'
    if(!$resolved.StartsWith($tempRoot,[StringComparison]::OrdinalIgnoreCase) -or
       [IO.Path]::GetFileName($resolved) -notlike 'patchwork-test-*') {throw 'Unsafe test cleanup path'}
    Remove-Item -LiteralPath $resolved -Recurse -Force
}
