param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^COM[0-9]+$')]
    [string]$Port
)
$ErrorActionPreference = 'Stop'
$imagePath = Join-Path $PSScriptRoot 'saishubako-cardputer-full.bin'
if (-not (Test-Path -LiteralPath $imagePath)) { throw 'Build or download saishubako-cardputer-full.bin first.' }
if (-not (Get-Command python -ErrorAction SilentlyContinue)) { throw 'Install Python first.' }
& python -m esptool version
if ($LASTEXITCODE -ne 0) { throw 'Run: python -m pip install esptool==4.7.0' }
Write-Host 'Enter download mode: hold G0, press RST once, wait one second, then release G0.'
& python -m esptool --chip esp32s3 --port $Port --before no_reset --baud 460800 write_flash 0x0 $imagePath
if ($LASTEXITCODE -ne 0) { throw 'Flash failed. Check the cable, port and download mode.' }
Write-Host 'Done. Press RST WITHOUT holding G0 to start SAISHUBAKO.'
