/**
 * @file app.c
 * @brief Main application implementation for EFR32MG1 Zigbee SED
 *
 * Production-quality Zigbee Sleepy End Device with SHT31 sensor
 */

#include "app.h"
#include "button.h"
#include "sht31.h"
#include "battery.h"

#include "af.h"
#include "app/framework/plugin/network-steering/network-steering.h"
#include "app/framework/plugin/reporting/reporting.h"
#include "stack/include/ember-types.h"
#include "sl_component_catalog.h"

#ifdef SL_CATALOG_SIMPLE_LED_PRESENT
#include "sl_simple_led_instances.h"
#endif

#ifdef SL_CATALOG_POWER_MANAGER_PRESENT
#include "sl_power_manager.h"
#endif

//==============================================================================
// Private Variables
//==============================================================================

static AppContext_t appContext = {
  .state = APP_STATE_INIT,
  .fastPollActive = false,
  .joinTimestamp = 0,
  .joinAttempts = 0,
  .sensorInitialized = false,
  .buttonPressed = false
};

static sl_sleeptimer_timer_handle_t sensorTimer;
static sl_sleeptimer_timer_handle_t fastPollTimer;

//==============================================================================
// Forward Declarations
//==============================================================================

static void sensor_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);
static void fast_poll_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);
static void transition_to_normal_poll(void);
static void print_network_info(void);
static void print_reset_info(void);

//==============================================================================
// Application Framework Callbacks
//==============================================================================

/**
 * @brief Application init callback
 * Called once at startup after stack initialization
 */
void emberAfMainInitCallback(void)
{
  app_init();
}

/**
 * @brief Main application tick
 * Called periodically
 */
void emberAfMainTickCallback(void)
{
  app_process_action();
}

/**
 * @brief Stack status callback
 */
void emberAfStackStatusCallback(EmberStatus status)
{
  app_stack_status_callback(status);
}

/**
 * @brief Complete network steering callback
 */
void emberAfPluginNetworkSteeringCompleteCallback(EmberStatus status,
                                                    uint8_t totalBeacons,
                                                    uint8_t joinAttempts,
                                                    uint8_t finalState)
{
  APP_LOG("Network steering complete: status=0x%02X, beacons=%d, attempts=%d, state=%d",
          status, totalBeacons, joinAttempts, finalState);

  if (status == EMBER_SUCCESS) {
    APP_LOG("Successfully joined network!");
    appContext.state = APP_STATE_JOINED_FAST_POLL;
    appContext.joinTimestamp = halCommonGetInt32uMillisecondTick();

    // Enable fast polling for smooth interview
    app_set_fast_poll(true);

    // Start fast poll timeout timer (30 seconds)
    sl_sleeptimer_start_timer_ms(&fastPollTimer,
                                  APP_FAST_POLL_TIMEOUT_MS,
                                  fast_poll_timer_callback,
                                  NULL,
                                  0,
                                  0);

    APP_LOG("Fast poll enabled for %d seconds", APP_FAST_POLL_TIMEOUT_MS / 1000);

  } else {
    APP_LOG("Join failed with status 0x%02X", status);
    appContext.state = APP_STATE_NOT_JOINED;
    appContext.joinAttempts++;
  }
}

//==============================================================================
// Public Functions - Application Logic
//==============================================================================

void app_init(void)
{
  APP_LOG("=================================================");
  APP_LOG("  EFR32MG1 Zigbee SED with SHT31");
  APP_LOG("  Version: %s", APP_SW_BUILD_ID);
  APP_LOG("  Manufacturer: %s", APP_MANUFACTURER_NAME);
  APP_LOG("  Model: %s", APP_MODEL_IDENTIFIER);
  APP_LOG("=================================================");

  print_reset_info();

  // Initialize hardware drivers
  APP_LOG("Initializing hardware...");

  // Initialize button
  button_init();
  APP_LOG("Button initialized on PB13");

  // Initialize SHT31 sensor
  appContext.sensorInitialized = sht31_init();
  if (appContext.sensorInitialized) {
    APP_LOG("SHT31 sensor initialized on I2C (PC10/PC11)");
  } else {
    APP_LOG("SHT31 sensor not found - using fallback values");
  }

  // Initialize battery monitor
  battery_init();
  APP_LOG("Battery monitor initialized");

  // Start periodic sensor reading timer
  sl_sleeptimer_start_periodic_timer_ms(&sensorTimer,
                                         APP_SENSOR_READ_PERIOD_MS,
                                         sensor_timer_callback,
                                         NULL,
                                         0,
                                         0);
  APP_LOG("Sensor timer started (period: %d ms)", APP_SENSOR_READ_PERIOD_MS);

  // Check network state
  EmberNetworkStatus networkStatus = emberAfNetworkState();
  if (networkStatus == EMBER_JOINED_NETWORK) {
    APP_LOG("Already joined to network");
    appContext.state = APP_STATE_JOINED_NORMAL;
    print_network_info();

    // Do initial sensor read
    app_update_sensor_data();
    app_update_battery_data();

  } else {
    APP_LOG("Not joined to any network");
    appContext.state = APP_STATE_NOT_JOINED;
    APP_LOG("Press BTN0 short to join or long press to force join");
  }

  APP_LOG("Application initialization complete");
}

void app_process_action(void)
{
  // Process any pending button events
  button_process();

  // State-specific processing
  switch (appContext.state) {
    case APP_STATE_INIT:
      // Should not stay here long
      break;

    case APP_STATE_NOT_JOINED:
      // Waiting for user to trigger join
      break;

    case APP_STATE_JOINING:
      // Network steering in progress
      break;

    case APP_STATE_JOINED_FAST_POLL:
      // Fast poll active during interview
      break;

    case APP_STATE_JOINED_NORMAL:
      // Normal operation
      break;

    case APP_STATE_LEAVING:
      // Leaving network
      break;
  }
}

AppState_t app_get_state(void)
{
  return appContext.state;
}

void app_start_join(void)
{
  if (appContext.state == APP_STATE_JOINING) {
    APP_LOG("Join already in progress");
    return;
  }

  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    APP_LOG("Already joined to network");
    return;
  }

  APP_LOG("Starting network join...");
  appContext.state = APP_STATE_JOINING;

  // Use network steering plugin
  EmberStatus status = emberAfPluginNetworkSteeringStart();
  if (status == EMBER_SUCCESS) {
    APP_LOG("Network steering started");
  } else {
    APP_LOG("Failed to start network steering: 0x%02X", status);
    appContext.state = APP_STATE_NOT_JOINED;
  }
}

void app_leave_network(void)
{
  if (emberAfNetworkState() != EMBER_JOINED_NETWORK) {
    APP_LOG("Not joined to any network");
    return;
  }

  APP_LOG("Leaving network...");
  appContext.state = APP_STATE_LEAVING;

  // Stop fast poll if active
  if (appContext.fastPollActive) {
    app_set_fast_poll(false);
  }

  EmberStatus status = emberLeaveNetwork();
  if (status == EMBER_SUCCESS) {
    APP_LOG("Leave request sent");
  } else {
    APP_LOG("Failed to leave network: 0x%02X", status);
  }
}

void app_set_fast_poll(bool enable)
{
  if (enable && !appContext.fastPollActive) {
    APP_LOG("Enabling fast poll (interval: %d ms)", APP_FAST_POLL_INTERVAL_QS * 250);
    emberAfSetWakeTimeoutQsCallback(APP_FAST_POLL_INTERVAL_QS);
    emberAfAddToCurrentAppTasksCallback(EMBER_AF_WAITING_FOR_DATA_ACK);
    appContext.fastPollActive = true;

  } else if (!enable && appContext.fastPollActive) {
    APP_LOG("Disabling fast poll, returning to normal (interval: %d ms)",
            APP_NORMAL_POLL_INTERVAL_QS * 250);
    emberAfSetWakeTimeoutQsCallback(APP_NORMAL_POLL_INTERVAL_QS);
    emberAfRemoveFromCurrentAppTasksCallback(EMBER_AF_WAITING_FOR_DATA_ACK);
    appContext.fastPollActive = false;
  }
}

void app_trigger_sensor_read(void)
{
  APP_LOG("Manual sensor read triggered");
  app_update_sensor_data();
  app_update_battery_data();
}

void app_update_sensor_data(void)
{
  int16_t temperature_raw;
  uint16_t humidity_raw;
  float temperature_celsius;
  float humidity_percent;

  // Read sensor
  bool success = sht31_read(&temperature_celsius, &humidity_percent);

  if (success || !appContext.sensorInitialized) {
    // Convert to ZCL format (temperature: 0.01°C, humidity: 0.01%)
    temperature_raw = (int16_t)(temperature_celsius * 100);
    humidity_raw = (uint16_t)(humidity_percent * 100);

    APP_LOG("Sensor: temp=%.2f°C, humidity=%.2f%%",
            temperature_celsius, humidity_percent);

    // Update ZCL attributes
    emberAfWriteServerAttribute(APP_ENDPOINT,
                                 ZCL_TEMP_MEASUREMENT_CLUSTER_ID,
                                 ZCL_TEMP_MEASURED_VALUE_ATTRIBUTE_ID,
                                 (uint8_t*)&temperature_raw,
                                 ZCL_INT16S_ATTRIBUTE_TYPE);

    emberAfWriteServerAttribute(APP_ENDPOINT,
                                 ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID,
                                 ZCL_RELATIVE_HUMIDITY_MEASURED_VALUE_ATTRIBUTE_ID,
                                 (uint8_t*)&humidity_raw,
                                 ZCL_INT16U_ATTRIBUTE_TYPE);

  } else {
    APP_ERROR("Failed to read sensor");
  }
}

void app_update_battery_data(void)
{
  uint16_t voltage_mv = battery_read_voltage();
  uint8_t percentage = battery_voltage_to_percentage(voltage_mv);

  // ZCL format: voltage in 100mV units, percentage in 0.5% units (0-200)
  uint8_t battery_voltage = voltage_mv / 100;
  uint8_t battery_percentage = percentage * 2;  // Convert to 0-200 range

  APP_LOG("Battery: %d mV (%d%%)", voltage_mv, percentage);

  // Update ZCL attributes
  emberAfWriteServerAttribute(APP_ENDPOINT,
                               ZCL_POWER_CONFIG_CLUSTER_ID,
                               ZCL_BATTERY_VOLTAGE_ATTRIBUTE_ID,
                               &battery_voltage,
                               ZCL_INT8U_ATTRIBUTE_TYPE);

  emberAfWriteServerAttribute(APP_ENDPOINT,
                               ZCL_POWER_CONFIG_CLUSTER_ID,
                               ZCL_BATTERY_PERCENTAGE_REMAINING_ATTRIBUTE_ID,
                               &battery_percentage,
                               ZCL_INT8U_ATTRIBUTE_TYPE);
}

void app_stack_status_callback(EmberStatus status)
{
  APP_LOG("Stack status: 0x%02X", status);

  switch (status) {
    case EMBER_NETWORK_UP:
      APP_LOG("Network UP");
      if (appContext.state != APP_STATE_JOINED_FAST_POLL &&
          appContext.state != APP_STATE_JOINED_NORMAL) {
        appContext.state = APP_STATE_JOINED_NORMAL;
        print_network_info();
      }
      break;

    case EMBER_NETWORK_DOWN:
      APP_LOG("Network DOWN");
      appContext.state = APP_STATE_NOT_JOINED;
      if (appContext.fastPollActive) {
        app_set_fast_poll(false);
      }
      break;

    case EMBER_JOIN_FAILED:
      APP_LOG("Join FAILED");
      appContext.state = APP_STATE_NOT_JOINED;
      break;

    default:
      break;
  }
}

//==============================================================================
// Private Functions
//==============================================================================

static void sensor_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;

  // Only update if joined to network
  if (appContext.state == APP_STATE_JOINED_FAST_POLL ||
      appContext.state == APP_STATE_JOINED_NORMAL) {
    app_update_sensor_data();
    app_update_battery_data();
  }
}

static void fast_poll_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;

  APP_LOG("Fast poll timeout - transitioning to normal poll");
  transition_to_normal_poll();
}

static void transition_to_normal_poll(void)
{
  if (appContext.state == APP_STATE_JOINED_FAST_POLL) {
    appContext.state = APP_STATE_JOINED_NORMAL;
    app_set_fast_poll(false);
    APP_LOG("Transitioned to normal operation mode");
  }
}

static void print_network_info(void)
{
  EmberNodeId nodeId = emberAfGetNodeId();
  EmberPanId panId = emberAfGetPanId();
  int8_t radioChannel = emberAfGetRadioChannel();

  APP_LOG("Network Info:");
  APP_LOG("  Node ID: 0x%04X", nodeId);
  APP_LOG("  PAN ID: 0x%04X", panId);
  APP_LOG("  Channel: %d", radioChannel);
}

static void print_reset_info(void)
{
  uint8_t resetCode = halGetResetInfo();
  APP_LOG("Last reset reason: 0x%02X", resetCode);

  switch (resetCode) {
    case EMBER_RESET_POWER_ON:
      APP_LOG("  (Power-on reset)");
      break;
    case EMBER_RESET_EXTERNAL:
      APP_LOG("  (External reset)");
      break;
    case EMBER_RESET_WATCHDOG:
      APP_LOG("  (Watchdog reset)");
      break;
    case EMBER_RESET_SOFTWARE:
      APP_LOG("  (Software reset)");
      break;
    default:
      APP_LOG("  (Unknown reset)");
      break;
  }
}

//==============================================================================
// Button Callbacks
//==============================================================================

void button_short_press_callback(void)
{
  APP_LOG("Button short press detected");

  if (appContext.state == APP_STATE_NOT_JOINED) {
    // Not joined - start join
    APP_LOG("Starting join process...");
    app_start_join();
  } else if (appContext.state == APP_STATE_JOINED_FAST_POLL ||
             appContext.state == APP_STATE_JOINED_NORMAL) {
    // Already joined - trigger sensor read
    APP_LOG("Triggering immediate sensor read...");
    app_trigger_sensor_read();

#ifdef SL_CATALOG_SIMPLE_LED_PRESENT
    // Flash LED to confirm
    sl_led_toggle(&sl_led_led0);
#endif
  }
}

void button_long_press_callback(void)
{
  APP_LOG("Button long press detected");

  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    // Joined - leave network
    APP_LOG("Leaving network...");
    app_leave_network();
  } else {
    // Not joined - force join
    APP_LOG("Force joining network...");
    app_start_join();
  }
}

//==============================================================================
// CLI Command Handlers
//==============================================================================

void cli_sensor_read(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  app_trigger_sensor_read();
}

void cli_battery_read(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  app_update_battery_data();
}

void cli_network_status(sl_cli_command_arg_t *arguments)
{
  (void)arguments;

  APP_LOG("=== Network Status ===");
  APP_LOG("State: %d", appContext.state);

  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    print_network_info();
    APP_LOG("Fast poll: %s", appContext.fastPollActive ? "enabled" : "disabled");
  } else {
    APP_LOG("Not joined to network");
    APP_LOG("Join attempts: %d", appContext.joinAttempts);
  }
}
