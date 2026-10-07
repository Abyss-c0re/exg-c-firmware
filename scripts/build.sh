#!/bin/sh
# Pull the AVR toolchain and NeuroPawn 1.2+ from their registries, then build.
# Writes .pio/build/knight/firmware.hex. That file is not committed.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
cd "$root"
if ! command -v pio >/dev/null 2>&1; then
    echo "pio is not installed. Install PlatformIO Core, then run this again."
    echo "https://docs.platformio.org/en/latest/core/installation.html"
    exit 1
fi
pio pkg install
pio run -e knight
echo "hex: $root/.pio/build/knight/firmware.hex"
