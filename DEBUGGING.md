# Debugging EFR32MG1 SED Firmware

This guide covers debugging techniques for the EFR32MG1 SED firmware.

## Debug Output Methods

### 1. Serial Console (UART/VCOM)
**Recommended for most debugging**

#### Setup
- Baud rate: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1
- Flow control: None

#### Connection
```bash
# Linux
screen /dev/ttyUSB0 115200

# macOS
screen /dev/cu.usbserial-* 115200

# Windows
# Use PuTTY or TeraTerm with COM port
```

#### Log Levels
The firmware uses several log macros:
- `APP_LOG()` - General information
- `APP_INFO()` - Important events
- `APP_ERROR()` - Errors
- `APP_DEBUG()` - Detailed debug info (verbose)

### 2. SWO (Serial Wire Output)
**Real-time trace without UART**

#### Advantages
- Non-intrusive (doesn't use UART pins)
- High bandwidth
- Precise timing information
- No impact on timing-critical code

#### Setup with Simplicity Commander
```bash
# Start SWO capture
commander swo read --speed 115200 --out swo.log

# In another terminal, tail the log
tail -f swo.log
```

#### Setup with J-Link
```bash
# Using J-Link SWO Viewer
JLinkSWOViewer -device EFR32MG1P132F256GM32 -itmport 0
```

### 3. GDB Debugging
**Interactive debugging with breakpoints**

#### Requirements
- Debug build (`build/debug/efr32mg1-sed.axf`)
- J-Link or compatible debugger
- ARM GDB (`arm-none-eabi-gdb`)

#### Using Simplicity Commander GDB Server
Terminal 1:
```bash
commander gdbserver start
```

Terminal 2:
```bash
arm-none-eabi-gdb build/debug/efr32mg1-sed.axf
(gdb) target remote localhost:2331
(gdb) monitor reset
(gdb) load
(gdb) break main
(gdb) continue
```

#### Using J-Link GDB Server
Terminal 1:
```bash
JLinkGDBServer -device EFR32MG1P132F256GM32 -if SWD -speed 4000
```

Terminal 2:
```bash
arm-none-eabi-gdb build/debug/efr32mg1-sed.axf
(gdb) target remote localhost:2331
(gdb) monitor reset
(gdb) load
(gdb) break app_init
(gdb) continue
```

#### Useful GDB Commands
```
break app.c:123         # Set breakpoint at line
break button_init       # Set breakpoint at function
info breakpoints        # List breakpoints
continue                # Resume execution
step                    # Step into function
next                    # Step over function
finish                  # Run until function returns
print variable          # Print variable value
print/x register        # Print in hex
backtrace               # Show call stack
info locals             # Show local variables
info registers          # Show CPU registers
```

## Common Debug Scenarios

### Network Join Issues

#### Enable Verbose Logging
Edit `app.h` and ensure debug prints are enabled:
```c
#define APP_DEBUG(...)  emberAfDebugPrintln(__VA_ARGS__)
```

#### Check Network Steering
Look for these log messages:
```
Network steering started
Network steering complete: status=0x00
```

If status is not 0x00, check:
- Coordinator is in pairing mode
- Correct security configuration
- No channel conflicts

#### Monitor Stack Events
All stack status changes are logged:
```
Stack status: 0x00  // EMBER_NETWORK_UP
Stack status: 0x01  // EMBER_NETWORK_DOWN
Stack status: 0xA3  // EMBER_JOIN_FAILED
```

### Interview Stalls

#### Check Fast Poll
Verify fast poll is enabled:
```
Fast poll enabled for 30 seconds
Enabling fast poll (interval: 200 ms)
```

If not, check `app_set_fast_poll()` is called after join.

#### Monitor Poll Activity
Enable poll debug in Ember:
```c
// In emberAfMainTickCallback
emberAfCorePrintln("Poll: %d ms", emberAfGetWakeTimeoutQsCallback() * 250);
```

#### Check Message Queue
```c
// Add to app_process_action
uint8_t pending = emberAfGetCurrentAppTasks();
if (pending != 0) {
    APP_DEBUG("Pending tasks: 0x%02X", pending);
}
```

### Sensor Read Failures

#### Enable I2C Debug
Add to `sht31.c`:
```c
static bool i2c_write_command(uint8_t cmd_msb, uint8_t cmd_lsb)
{
    APP_DEBUG("I2C write: cmd=0x%02X%02X", cmd_msb, cmd_lsb);
    // ... existing code ...
    APP_DEBUG("I2C write result: %d", ret);
    return (ret == i2cTransferDone);
}
```

#### Check Fallback Mode
If sensor is not present:
```
SHT31 sensor NOT detected - using fallback mode
Fallback values: temp=22.50°C, humidity=45.30% (count=1)
```

#### Verify I2C Pins
Use oscilloscope or logic analyzer:
- SDA: PC10 (Pin 24)
- SCL: PC11 (Pin 25)
- Check for clock signal
- Check for ACK/NAK

### Button Not Responding

#### Check GPIO Configuration
Add to `button_init()`:
```c
APP_DEBUG("Button pin state: %d", GPIO_PinInGet(BUTTON_PORT, BUTTON_PIN));
```

Expected: 1 (high) when not pressed, 0 (low) when pressed

#### Monitor Interrupts
Add to `button_gpio_callback()`:
```c
static void button_gpio_callback(uint8_t intNo)
{
    APP_DEBUG("Button IRQ: int=%d, state=%d",
              intNo, GPIO_PinInGet(BUTTON_PORT, BUTTON_PIN));
    buttonContext.interruptPending = true;
}
```

#### Check State Machine
Add to `button_process()`:
```c
static ButtonState_t lastState = BUTTON_STATE_IDLE;
if (buttonContext.state != lastState) {
    APP_DEBUG("Button state: %d -> %d", lastState, buttonContext.state);
    lastState = buttonContext.state;
}
```

### Memory Issues

#### Check Stack Usage
Add to `app.c`:
```c
extern uint32_t __StackTop;
extern uint32_t __StackLimit;

void check_stack_usage(void)
{
    uint32_t stackSize = (uint32_t)&__StackTop - (uint32_t)&__StackLimit;
    uint32_t stackUsed = stackSize - emberAfGetAvailableStack();
    APP_DEBUG("Stack: used=%lu, total=%lu (%lu%%)",
              stackUsed, stackSize, (stackUsed * 100) / stackSize);
}
```

#### Check Heap Usage
```c
void check_heap_usage(void)
{
    uint16_t available = emberAfGetHeapAvailable();
    APP_DEBUG("Heap available: %u bytes", available);
}
```

### Performance Profiling

#### Measure Function Execution Time
```c
uint32_t start = halCommonGetInt32uMillisecondTick();
// ... function to profile ...
uint32_t duration = halCommonGetInt32uMillisecondTick() - start;
APP_DEBUG("Function took %lu ms", duration);
```

#### Measure Sensor Read Time
Already implemented in `sht31_read()`:
```c
uint32_t start = get_time_ms();
// ... I2C transaction ...
uint32_t duration = get_time_ms() - start;
APP_DEBUG("Sensor read took %lu ms", duration);
```

## Advanced Debugging

### Network Packet Sniffer

#### Using Wireshark with Zigbee Sniffer
1. Get USB Zigbee sniffer (e.g., CC2531 with sniffer firmware)
2. Install Wireshark with Zigbee dissector
3. Capture packets during join/interview
4. Look for:
   - Device Announce
   - Active Endpoint Request/Response
   - Simple Descriptor Request/Response
   - Bind Request/Response
   - Configure Reporting Request/Response

### Energy Mode Profiling

#### Using Simplicity Studio Energy Profiler
1. Connect target via J-Link
2. Open Simplicity Studio
3. Launch Energy Profiler
4. Monitor:
   - Current consumption over time
   - Energy mode transitions (EM0→EM1→EM2)
   - Average current
   - Peak current

Expected values:
- Active (EM0): 10-30 mA
- Sleep (EM2): 10-100 µA
- TX/RX: 20-40 mA peaks

### RTT (Real-Time Transfer)

#### Alternative to SWO
RTT provides bidirectional communication channel.

Add to project:
```c
#include "SEGGER_RTT.h"

// Use instead of printf
SEGGER_RTT_printf(0, "Temperature: %.2f\n", temp);
```

View output:
```bash
JLinkRTTClient
```

## Troubleshooting Common Issues

### Issue: No Serial Output

**Check:**
1. Correct COM port selected
2. Baud rate is 115200
3. UART not disabled in config
4. VCOM pins connected

**Test:**
```bash
# Send command and see if echo works
echo "network_status" > /dev/ttyUSB0
```

### Issue: Device Keeps Resetting

**Check reset reason:**
```
Last reset reason: 0x03 (Watchdog reset)
```

Possible causes:
- Infinite loop
- Stack overflow
- Interrupt storm

**Debug:**
1. Check stack usage
2. Look for blocking code
3. Verify interrupt handlers return quickly

### Issue: I2C Hangs

**Symptoms:**
- Device stops responding
- No sensor readings

**Debug:**
1. Check I2C bus not stuck
2. Add timeout to I2C transfers
3. Verify pull-up resistors on SDA/SCL

### Issue: High Current Consumption

**Expected:** <100µA average with 7.5s poll

**Debug:**
1. Check sleep mode entered: `APP_DEBUG("Entering EM%d", mode);`
2. Verify fast poll disabled after timeout
3. Check for peripherals not disabled (LEDs, ADC, etc.)
4. Monitor with Energy Profiler

## Debug Build vs Release Build

### Debug Build
- Optimization: -O0 or -Og
- Debug symbols: Yes
- Size: Larger
- Speed: Slower
- Debugging: Easy
- Use for: Development, debugging

### Release Build
- Optimization: -Os (size)
- Debug symbols: No
- Size: Smaller
- Speed: Faster
- Debugging: Harder
- Use for: Production, performance testing

## Best Practices

1. **Use appropriate log level**
   - Don't use APP_DEBUG in production
   - Reserve APP_ERROR for actual errors

2. **Add context to logs**
   - Bad: `APP_LOG("Failed");`
   - Good: `APP_LOG("Sensor read failed: I2C error 0x%02X", error);`

3. **Log state transitions**
   - Network state changes
   - Button state changes
   - Power mode changes

4. **Use unique error codes**
   ```c
   #define ERR_I2C_TIMEOUT    0x01
   #define ERR_SENSOR_CRC     0x02
   #define ERR_NETWORK_FAIL   0x03
   ```

5. **Preserve logs across resets**
   - Consider storing critical errors in NVM3
   - Implement crash dump mechanism

## Additional Resources

- [Silicon Labs Community Forums](https://community.silabs.com/)
- [Gecko SDK Documentation](https://docs.silabs.com/gecko-platform/)
- [Zigbee Specification](https://zigbeealliance.org/developer_resources/)
