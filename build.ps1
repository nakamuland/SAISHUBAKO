$ErrorActionPreference = 'Stop'
if (-not (Get-Command python -ErrorAction SilentlyContinue)) { throw 'Install Python first.' }
$env:PYTHONIOENCODING = 'utf-8'
& python -m platformio run --project-dir $PSScriptRoot
if ($LASTEXITCODE -ne 0) { throw 'Build failed. See the compiler output above.' }
Write-Host 'Build complete: saishubako-cardputer-full.bin'
