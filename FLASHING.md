# Flashing EFR32MG1 SED Firmware

This guide covers flashing the firmware to EFR32MG1 devices.

## Prerequisites

### Hardware
- EFR32MG1P132F256GM32 module (TRADFRI-compatible)
- J-Link debug probe OR Simplicity Studio Wireless Starter Kit
- USB cable
- Power supply (2x AA batteries or external 3V)

### Software
- Simplicity Commander (recommended) OR
- J-Link tools OR
- OpenOCD

## Method 1: Simplicity Commander (Recommended)

### Installation

#### Windows
Download from: https://www.silabs.com/developers/mcu-programming-options
- Install Simplicity Studio 5 (includes Commander)
- Or download standalone Commander

#### Linux
```bash
# Download standalone Commander
wget https://www.silabs.com/documents/login/software/SimplicityCommander-Linux.zip
unzip SimplicityCommander-Linux.zip
sudo dpkg -i SimplicityCommander.deb
```

#### macOS
```bash
# Download and install from Silicon Labs website
# Or use Homebrew (if available):
brew install --cask simplicity-commander
```

### Flashing Procedure

#### Step 1: Connect Hardware
1. Connect J-Link probe to EFR32MG1 SWD pins:
   - SWCLK
   - SWDIO
   - GND
   - VCC (3.3V)
2. Connect J-Link to computer via USB
3. Power the target (batteries or external supply)

#### Step 2: Verify Connection
```bash
# List connected devices
commander device list

# Expected output:
# J-Link Serial No.: 123456789
# Target: EFR32MG1P132F256GM32
```

#### Step 3: Full Flash Procedure (First Time)

**Important**: First-time flashing requires mass erase to clear any existing firmware and security settings.

```bash
# 1. Mass erase (clears everything including user data)
commander device masserase

# 2. Flash the firmware (use .s37 format)
commander flash build/release/efr32mg1-sed.s37

# 3. Verify flash
commander verify build/release/efr32mg1-sed.s37

# 4. Reset device
commander device reset
```

#### Step 4: Update Flash (Subsequent Flashes)

If you're updating existing firmware:
```bash
# Flash without erase (preserves NVM3 tokens)
commander flash build/release/efr32mg1-sed.s37 --noreset

# Reset to start new firmware
commander device reset
```

### Flash Script

Create `flash.sh` for convenience:
```bash
#!/bin/bash
set -e

FIRMWARE=${1:-build/release/efr32mg1-sed.s37}
MODE=${2:-update}  # update or full

if [ "$MODE" == "full" ]; then
    echo "Full flash (with mass erase)..."
    commander device masserase
fi

echo "Flashing $FIRMWARE..."
commander flash "$FIRMWARE"

echo "Verifying..."
commander verify "$FIRMWARE"

echo "Resetting device..."
commander device reset

echo "Done!"
```

Usage:
```bash
chmod +x flash.sh
./flash.sh                                    # Update existing firmware
./flash.sh build/release/efr32mg1-sed.s37 full  # Full flash with erase
```

## Method 2: J-Link Tools

### Installation
Download from: https://www.segger.com/downloads/jlink/

### Flashing
```bash
# Create flash script (flash.jlink)
cat > flash.jlink << EOF
connect
device EFR32MG1P132F256GM32
si SWD
speed 4000
loadfile build/release/efr32mg1-sed.hex
r
g
exit
EOF

# Execute
JLinkExe -CommanderScript flash.jlink
```

## Method 3: OpenOCD (Advanced)

### Configuration
Create `openocd.cfg`:
```
source [find interface/jlink.cfg]
transport select swd

source [find target/efm32.cfg]

adapter_khz 1000

init
reset halt
```

### Flashing
```bash
# Flash
openocd -f openocd.cfg -c "program build/release/efr32mg1-sed.hex verify reset exit"
```

## Debugging with SWO (Serial Wire Output)

SWO provides real-time debug output without UART.

### Enable SWO in Firmware
Already enabled in debug builds. Uses ITM channels for printf-style logging.

### Capture SWO Output
```bash
# Using Commander
commander swo read --speed 115200 --out swo.log

# Using J-Link
JLinkSWOViewer -device EFR32MG1P132F256GM32 -itmport 0
```

## Serial Console Access

The firmware outputs debug logs via UART (VCOM on debug connector).

### Connection Settings
- Baud rate: 115200
- Data bits: 8
- Parity: None
- Stop bits: 1
- Flow control: None

### Linux
```bash
# Find device
ls /dev/ttyUSB* /dev/ttyACM*

# Connect with screen
screen /dev/ttyUSB0 115200

# Or with minicom
minicom -D /dev/ttyUSB0 -b 115200

# Exit: Ctrl-A, K (screen) or Ctrl-A, X (minicom)
```

### Windows
Use PuTTY or TeraTerm:
1. Select Serial
2. COM port (check Device Manager)
3. Speed: 115200

### macOS
```bash
# Find device
ls /dev/cu.*

# Connect
screen /dev/cu.usbserial-* 115200
```

## Troubleshooting

### Device Not Found
**Problem**: "Could not connect to J-Link" or "No target device found"

**Solutions**:
1. Check USB connection
2. Verify power to target
3. Check SWD connections (SWCLK, SWDIO, GND)
4. Update J-Link firmware:
   ```bash
   commander jlink upgrade
   ```

### Flash Verification Failed
**Problem**: "Verification failed at address 0x..."

**Solutions**:
1. Reduce SWD speed:
   ```bash
   commander flash --speed 1000 build/release/efr32mg1-sed.s37
   ```
2. Check power supply stability
3. Try mass erase first

### Device Locked
**Problem**: "Device is locked" or "Flash protection enabled"

**Solution**:
```bash
# Unlock device (WARNING: Erases everything)
commander device unlock --debug on

# Then mass erase
commander device masserase
```

### Cannot Enter Debug Mode
**Problem**: Device constantly resetting or not responding

**Solutions**:
1. Hold device in reset while connecting
2. Use "connect under reset":
   ```bash
   commander flash --noreset --halt build/release/efr32mg1-sed.s37
   ```
3. Power cycle the device

### Serial Console Shows Garbage
**Problem**: Random characters or no output

**Solutions**:
1. Verify baud rate (115200)
2. Check UART pins not shorted
3. Ensure correct COM port
4. Try different terminal program

## Production Flashing

For production:
1. Use release build (optimized, no debug symbols)
2. Always verify after flashing
3. Consider using .s37 format (includes checksums)
4. Log serial numbers and flash dates
5. Test basic functionality after flashing

### Batch Flashing Script
```bash
#!/bin/bash
LOG_FILE="flash_log_$(date +%Y%m%d_%H%M%S).txt"

for i in {1..10}; do
    echo "Flashing device $i..."
    commander device masserase
    commander flash build/release/efr32mg1-sed.s37
    commander verify build/release/efr32mg1-sed.s37

    if [ $? -eq 0 ]; then
        echo "Device $i: SUCCESS" | tee -a $LOG_FILE
    else
        echo "Device $i: FAILED" | tee -a $LOG_FILE
    fi

    echo "Remove device and press Enter for next..."
    read
done
```

## Next Steps

After flashing:
1. Open serial console to verify firmware is running
2. Check for initialization messages
3. Follow [VALIDATION.md](VALIDATION.md) for testing
