param([string]$OpenOcd='D:\Embedded_Toolchain\OpenOCD\xpack-openocd-0.12.0-7\bin\openocd.exe')
$ErrorActionPreference='Stop'
$buildDir=Join-Path $PSScriptRoot 'build_dma_irq'
$firmware=Join-Path $buildDir 'Smart_Weather_Clock.hex'
if (-not (Test-Path -LiteralPath $firmware)) { throw 'Firmware missing: build with cmake --build build_dma_irq first.' }
if (-not (Test-Path -LiteralPath $OpenOcd)) { throw "OpenOCD not found: $OpenOcd" }
Push-Location -LiteralPath $buildDir
try {
    # Native OpenOCD progress uses stderr; it is not a PowerShell script error.
    $ErrorActionPreference='Continue'
    & $OpenOcd -f interface/stlink.cfg -c 'transport select swd' -f target/stm32f4x.cfg -c 'program Smart_Weather_Clock.hex verify reset exit'
    $code=$LASTEXITCODE
    $ErrorActionPreference='Stop'
    if ($code -ne 0) { throw "Flashing failed (exit $code)." }
    Write-Host 'Flashed and verified. Board reset to run the DMA interrupt firmware.'
} finally { Pop-Location }
