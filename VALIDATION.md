# Validation Checklist for EFR32MG1 SED Firmware

This document provides a comprehensive testing checklist to verify the firmware functionality.

## Prerequisites

- [ ] Firmware flashed successfully
- [ ] Serial console connected (115200 baud)
- [ ] Zigbee coordinator available (ZHA or Zigbee2MQTT)
- [ ] SHT31 sensor connected (or using fallback mode)
- [ ] 2x AA batteries installed

## Phase 1: Basic Functionality

### 1.1 Power-On and Initialization
- [ ] Device powers on successfully
- [ ] Serial console shows initialization messages
- [ ] Reset reason displayed correctly
- [ ] Hardware initialization completes without errors
- [ ] SHT31 sensor detected (or fallback mode activated)
- [ ] Battery monitor initialized

Expected serial output:
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
Application initialization complete
```

### 1.2 Button Operation (Before Join)
- [ ] Short press on PB13 is detected
- [ ] Serial console shows "Button short press detected"
- [ ] Network steering starts
- [ ] Long press on PB13 is detected
- [ ] Serial console shows "Button long press detected"
- [ ] No duplicate/false triggers observed

### 1.3 LED Operation
- [ ] LED0 (PA0) responds during identify command
- [ ] LED blinks during button press (if implemented)
- [ ] LED behavior is consistent

## Phase 2: Network Join and Interview

### 2.1 Join Process
- [ ] Put coordinator in pairing mode
- [ ] Press button short on device
- [ ] Device starts network steering (check serial console)
- [ ] Device finds available networks
- [ ] Device successfully joins network
- [ ] Device announce sent
- [ ] Serial console shows "Successfully joined network!"

Expected serial output:
```
Button short press detected
Starting join process...
Network steering started
[...]
Network steering complete: status=0x00, beacons=X, attempts=Y, state=Z
Successfully joined network!
Fast poll enabled for 30 seconds
Network Info:
  Node ID: 0xXXXX
  PAN ID: 0xXXXX
  Channel: XX
```

### 2.2 Fast Poll Window
- [ ] Fast poll activates after join (check serial console)
- [ ] Fast poll duration is 30 seconds
- [ ] Poll interval is 200ms during fast poll
- [ ] Device stays awake during fast poll window

Expected serial output:
```
Fast poll enabled for 30 seconds
Enabling fast poll (interval: 200 ms)
```

### 2.3 Interview Completion
- [ ] Coordinator detects device
- [ ] Coordinator reads Basic cluster attributes:
  - Manufacturer: "faronov"
  - Model: "EFR32MG1-SED-SHT31"
  - Software version: "1.0.0"
- [ ] Coordinator discovers all clusters
- [ ] Temperature cluster appears
- [ ] Humidity cluster appears
- [ ] Power Configuration cluster appears
- [ ] Interview completes successfully (no timeout/stall)

### 2.4 Transition to Normal Poll
- [ ] After 30 seconds, fast poll timeout occurs
- [ ] Device transitions to normal poll mode
- [ ] Serial console shows "Fast poll timeout - transitioning to normal poll"
- [ ] Poll interval changes to 7.5 seconds
- [ ] Device continues to communicate with coordinator

Expected serial output:
```
Fast poll timeout - transitioning to normal poll
Disabling fast poll, returning to normal (interval: 7500 ms)
Transitioned to normal operation mode
```

## Phase 3: Sensor and Reporting

### 3.1 Periodic Measurements
- [ ] Sensor readings occur every 10 seconds (check serial console)
- [ ] Temperature values are reasonable (-40 to 125°C)
- [ ] Humidity values are reasonable (0 to 100%)
- [ ] Battery voltage is reasonable (2.0 to 3.2V)
- [ ] Battery percentage is calculated correctly

Expected serial output (every 10 seconds):
```
Sensor: temp=22.50°C, humidity=45.30%
Battery: 3000 mV (83%)
```

### 3.2 Attribute Reporting Configuration
- [ ] Configure reporting from coordinator for temperature:
  - Min interval: 30 seconds
  - Max interval: 300 seconds
  - Reportable change: 0.1°C (10 in ZCL units)
- [ ] Configure reporting from coordinator for humidity:
  - Min interval: 30 seconds
  - Max interval: 300 seconds
  - Reportable change: 1% (100 in ZCL units)
- [ ] Configuration accepted by device
- [ ] Serial console shows "Configure reporting command received"

### 3.3 Attribute Reports
- [ ] Temperature reports received by coordinator
- [ ] Humidity reports received by coordinator
- [ ] Battery reports received by coordinator
- [ ] Reports continue after fast poll window ends
- [ ] Report timing follows configured intervals
- [ ] Reports triggered by reportable change threshold
- [ ] Device does NOT "go silent" after interview

### 3.4 Manual Sensor Read
- [ ] Press button short while joined
- [ ] Immediate sensor reading triggered
- [ ] Serial console shows "Manual sensor read triggered"
- [ ] LED flashes briefly
- [ ] Values updated in coordinator

## Phase 4: Button Operations (While Joined)

### 4.1 Short Press
- [ ] Press button short (< 1 second)
- [ ] Serial console shows "Button short press detected"
- [ ] Serial console shows "Triggering immediate sensor read..."
- [ ] Sensor values updated immediately
- [ ] No unwanted side effects

### 4.2 Long Press
- [ ] Press and hold button (>= 3 seconds)
- [ ] Serial console shows "Button long press detected"
- [ ] Serial console shows "Leaving network..."
- [ ] Device leaves network
- [ ] Device disappears from coordinator
- [ ] Serial console shows "Network DOWN"

## Phase 5: Rejoin

### 5.1 Rejoin After Leave
- [ ] Device left network (via long press)
- [ ] Press button short to rejoin
- [ ] Network steering starts again
- [ ] Device rejoins network (may be faster if coordinator remembers device)
- [ ] Fast poll window activates again
- [ ] Interview completes
- [ ] Device resumes reporting

### 5.2 Persistent Data
- [ ] After rejoin, reporting configuration is remembered (if stored in NVM3)
- [ ] Device retains network keys (if not mass erased)

## Phase 6: ZHA-Specific Tests

### 6.1 ZHA Device Detection
- [ ] Device appears in ZHA with correct manufacturer
- [ ] Device appears in ZHA with correct model
- [ ] Device shows as "available" (not "unavailable")
- [ ] All entities created:
  - Temperature sensor
  - Humidity sensor
  - Battery voltage
  - Battery percentage

### 6.2 ZHA Configuration
- [ ] Can configure reporting intervals from ZHA UI
- [ ] Changes take effect on device
- [ ] Can trigger identify command from ZHA
- [ ] LED blinks during identify

### 6.3 ZHA Stability
- [ ] Device remains available for extended period (hours)
- [ ] No unexpected "unavailable" status
- [ ] Regular reports continue
- [ ] No coordinator log errors related to device

## Phase 7: Zigbee2MQTT-Specific Tests

### 7.1 Z2M Device Detection
- [ ] Device appears in Zigbee2MQTT
- [ ] Correct manufacturer and model shown
- [ ] Device interview completes successfully
- [ ] All sensors/entities exposed

### 7.2 Z2M Configuration
- [ ] Can configure reporting from Z2M UI
- [ ] Can read current values on demand
- [ ] Can trigger identify command
- [ ] Device responds correctly

### 7.3 Z2M Stability
- [ ] Device shows as "reachable"
- [ ] Link quality (LQI) is reasonable
- [ ] Regular updates continue
- [ ] No errors in Z2M logs

## Phase 8: Power Management

### 8.1 Sleep Behavior
- [ ] Device enters sleep mode between polls (if monitoring current)
- [ ] Device wakes on poll interval
- [ ] Device wakes on button press
- [ ] Current consumption is low during sleep (<100µA typical)

### 8.2 Battery Life Estimation
- [ ] Measure average current consumption
- [ ] Estimate battery life with 2xAA (≈3000mAh)
- [ ] Expected: >6 months with 7.5s poll interval

## Phase 9: Edge Cases and Error Handling

### 9.1 Sensor Failure
If SHT31 is disconnected:
- [ ] Device switches to fallback mode
- [ ] Serial console indicates "using fallback values"
- [ ] Fake values are generated (slow drift pattern)
- [ ] Device continues to operate normally
- [ ] Reports still sent to coordinator

### 9.2 Network Issues
- [ ] Coordinator powered off temporarily
- [ ] Device continues polling
- [ ] Device recovers when coordinator returns
- [ ] No crashes or hangs observed

### 9.3 Multiple Join Attempts
- [ ] If join fails, device returns to not-joined state
- [ ] Can retry join without reset
- [ ] Join attempt counter increments
- [ ] No resource leaks after failed attempts

### 9.4 Button Debouncing
- [ ] Rapid button presses are handled correctly
- [ ] No duplicate events from single press
- [ ] Bouncy button does not cause issues

## Phase 10: CLI Commands

### 10.1 sensor_read Command
- [ ] Execute `sensor_read` in serial console
- [ ] Immediate sensor reading occurs
- [ ] Values displayed in console
- [ ] Values updated in coordinator

### 10.2 battery_read Command
- [ ] Execute `battery_read` in serial console
- [ ] Battery voltage and percentage displayed
- [ ] Values are reasonable

### 10.3 network_status Command
- [ ] Execute `network_status` in serial console
- [ ] Current state displayed
- [ ] Network info shown (if joined)
- [ ] Fast poll status shown

## Phase 11: Long-Term Stability

### 11.1 Extended Operation (24 Hours)
- [ ] Device runs continuously for 24 hours
- [ ] No crashes or resets observed
- [ ] Reports continue regularly
- [ ] Memory usage stable (no leaks)
- [ ] Device remains responsive

### 11.2 Extended Operation (1 Week)
- [ ] Device runs for 1 week
- [ ] Battery percentage decreases appropriately
- [ ] No unexpected behavior
- [ ] Reports remain consistent

## Known Issues and Limitations

Document any issues found during validation:

### Issue Template
```
**Issue**: [Brief description]
**Severity**: Critical / Major / Minor / Cosmetic
**Steps to Reproduce**:
1. [Step 1]
2. [Step 2]
3. [Result]
**Expected**: [What should happen]
**Actual**: [What actually happens]
**Workaround**: [If available]
**Status**: Open / In Progress / Fixed
```

## Test Results Summary

| Phase | Test | Status | Notes |
|-------|------|--------|-------|
| 1.1 | Power-On | ☐ Pass / ☐ Fail | |
| 1.2 | Button (Before Join) | ☐ Pass / ☐ Fail | |
| 1.3 | LED | ☐ Pass / ☐ Fail | |
| 2.1 | Join Process | ☐ Pass / ☐ Fail | |
| 2.2 | Fast Poll | ☐ Pass / ☐ Fail | |
| 2.3 | Interview | ☐ Pass / ☐ Fail | |
| 2.4 | Normal Poll Transition | ☐ Pass / ☐ Fail | |
| 3.1 | Periodic Measurements | ☐ Pass / ☐ Fail | |
| 3.2 | Report Configuration | ☐ Pass / ☐ Fail | |
| 3.3 | Attribute Reports | ☐ Pass / ☐ Fail | |
| 3.4 | Manual Sensor Read | ☐ Pass / ☐ Fail | |
| 4.1 | Short Press (Joined) | ☐ Pass / ☐ Fail | |
| 4.2 | Long Press | ☐ Pass / ☐ Fail | |
| 5.1 | Rejoin | ☐ Pass / ☐ Fail | |
| 6.x | ZHA Tests | ☐ Pass / ☐ Fail | |
| 7.x | Z2M Tests | ☐ Pass / ☐ Fail | |
| 8.x | Power Management | ☐ Pass / ☐ Fail | |
| 9.x | Edge Cases | ☐ Pass / ☐ Fail | |
| 10.x | CLI Commands | ☐ Pass / ☐ Fail | |
| 11.x | Long-Term Stability | ☐ Pass / ☐ Fail | |

**Overall Status**: ☐ Pass / ☐ Fail / ☐ Partial

**Tester**: ___________________
**Date**: ___________________
**Firmware Version**: ___________________

## Conclusion

[Summary of test results and any critical issues]
