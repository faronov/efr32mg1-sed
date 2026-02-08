# EFR32MG1 Zigbee Sleepy End Device with SHT31

Production-quality Zigbee 3.0 Sleepy End Device (SED) firmware for EFR32MG1 with SHT31 temperature/humidity sensor support. Designed for stable operation with ZHA (Home Assistant) and Zigbee2MQTT coordinators.

## Features

### Zigbee Functionality
- ✅ **Zigbee 3.0 Sleepy End Device** with proper power management
- ✅ **Stable interview process** with fast poll window (30s @ 200ms)
- ✅ **Configurable attribute reporting** via coordinator
- ✅ **Network steering** for automatic joining
- ✅ **Install code support** for secure commissioning

### Clusters Implemented
- **Basic (0x0000)**: Manufacturer info, model, software version
- **Identify (0x0003)**: LED blinking for device identification
- **Power Configuration (0x0001)**: Battery voltage and percentage
- **Temperature Measurement (0x0402)**: SHT31 temperature readings
- **Relative Humidity Measurement (0x0405)**: SHT31 humidity readings

### Hardware Support
- **SHT31 Sensor**: I2C temperature/humidity sensor with fallback mode
- **Button (PB13)**: Short press (join/read) and long press (join/leave)
- **LED (PA0)**: Visual feedback for identify and button presses
- **Battery Monitor**: 2xAA battery voltage measurement via ADC

### Power Management
- Fast poll mode during interview: 200ms for 30 seconds after join
- Normal poll mode: 7.5 seconds for efficient battery life
- Automatic transition after interview completion
- Sleep manager integration for EM2 deep sleep

## Hardware Configuration

### Target Device
- **Part**: EFR32MG1P132F256GM32
- **Module**: TRADFRI-compatible module
- **Flash**: 256 KB
- **RAM**: 32 KB

### Pin Mapping
```
BTN0: PB13 (Pin 19) - Button input with pull-up
LED0: PA0  (Pin 2)  - Status LED
SDA:  PC10 (Pin 24) - I2C Data
SCL:  PC11 (Pin 25) - I2C Clock
```

### Battery
- Type: 2x AA alkaline batteries
- Voltage range: 2.0V - 3.2V
- Nominal: 3.0V

## Quick Start

### Building
See [BUILDING.md](BUILDING.md) for detailed build instructions.

```bash
# Build with Docker (recommended)
docker build -t efr32mg1-sed-builder .
docker run --rm -v $(pwd)/build:/workspace/build efr32mg1-sed-builder

# Artifacts will be in build/debug and build/release
```

### Flashing
See [FLASHING.md](FLASHING.md) for detailed flashing instructions.

```bash
# Using Simplicity Commander
commander device masserase
commander flash build/release/efr32mg1-sed.s37
```

### Testing
See [VALIDATION.md](VALIDATION.md) for the validation checklist.

## Button Operation

### Short Press (< 1 second)
- **When not joined**: Start network join process
- **When joined**: Trigger immediate sensor reading and report

### Long Press (>= 3 seconds)
- **When not joined**: Start network join process
- **When joined**: Leave network

## Network Behavior

### Join Process
1. User presses button (short or long)
2. Device starts network steering
3. Searches for available networks
4. Joins network with security (install code or well-known key)
5. Sends device announce
6. Enables **fast poll mode** (200ms) for 30 seconds
7. Completes interview with coordinator
8. Transitions to **normal poll mode** (7.5s)

### Reporting
- **Temperature**: Default min=30s, max=300s, reportable change=0.1°C
- **Humidity**: Default min=30s, max=300s, reportable change=1%
- **Battery**: Default min=3600s, max=86400s, reportable change=5%
- All reporting intervals can be configured by coordinator

### Sensor Readings
- Periodic measurements every 10 seconds
- Reports sent based on configured intervals
- Immediate reading on button short press

## Debugging

### Serial Output
The firmware outputs debug information via UART (VCOM):
- Baud rate: 115200
- Format: 8N1
- Pin: VCOM on debug connector

### Log Messages
```
=================================================
  EFR32MG1 Zigbee SED with SHT31
  Version: 1.0.0
  Manufacturer: faronov
  Model: EFR32MG1-SED-SHT31
=================================================
Last reset reason: 0x01 (Power-on reset)
Initializing hardware...
Button initialized on PB13
SHT31 sensor initialized on I2C (PC10/PC11)
Battery monitor initialized
Sensor timer started (period: 10000 ms)
Not joined to any network
Press BTN0 short to join or long press to force join
Application initialization complete
```

### CLI Commands
Access via serial console:
```
sensor_read    - Trigger immediate sensor reading
battery_read   - Read battery voltage
network_status - Display network status
```

## Project Structure

```
efr32mg1-sed/
├── src/
│   ├── app.c              # Main application
│   ├── app.h
│   ├── zcl_callbacks.c    # Zigbee cluster handlers
│   ├── button.c           # Button driver
│   ├── button.h
│   ├── sht31.c            # SHT31 sensor driver
│   ├── sht31.h
│   ├── battery.c          # Battery monitor
│   └── battery.h
├── config/
│   └── (generated files)
├── autogen/
│   └── (generated files)
├── .github/
│   └── workflows/
│       └── build-docker.yml
├── efr32mg1-sed.slcp      # Project configuration
├── Dockerfile
├── docker-build.sh
├── README.md
├── BUILDING.md
├── FLASHING.md
└── VALIDATION.md
```

## Known Limitations

1. **OTA Updates**: Not yet implemented
2. **Binding**: Find & Bind is included but not fully tested
3. **Groups**: Not implemented
4. **Scenes**: Not implemented

## Troubleshooting

### Device won't join
- Check button operation (LED should respond)
- Verify coordinator is in pairing mode
- Check serial output for error messages
- Try long press (force join)

### Interview fails/stalls
- Fast poll window is designed to prevent this
- Check coordinator logs
- Verify network is Zigbee 3.0 compatible

### Sensor not detected
- Firmware automatically switches to fallback mode
- Check I2C connections (PC10/PC11)
- Verify SHT31 address (0x44)

### Device stops reporting
- This should not happen with proper fast→normal poll transition
- Check poll configuration
- Verify device is still on network (network_status CLI command)

## Contributing

This is a demonstration project. For issues or questions:
- Open an issue on GitHub
- Check [VALIDATION.md](VALIDATION.md) for known issues

## License

MIT License - See LICENSE file for details

## Author

**faronov** - https://github.com/faronov

Built with Gecko SDK 4.5.0 and Ember AF 7.x
