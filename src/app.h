/**
 * @file app.h
 * @brief Main application header for EFR32MG1 Zigbee SED
 *
 * Production-quality Zigbee Sleepy End Device with SHT31 sensor
 *
 * @author Silicon Labs / Claude Code
 * @date 2026
 */

#ifndef APP_H
#define APP_H

#include <stdint.h>
#include <stdbool.h>

// Zigbee Application Framework
#include "af.h"
#include "af-types.h"
#include "app/framework/include/af.h"

//==============================================================================
// Application Configuration
//==============================================================================

#define APP_ENDPOINT                    1
#define APP_DEVICE_ID                   0x0302  // Temperature Sensor
#define APP_PROFILE_ID                  0x0104  // Home Automation

// Manufacturer Information
#define APP_MANUFACTURER_NAME           "faronov"
#define APP_MODEL_IDENTIFIER            "EFR32MG1-SED-SHT31"
#define APP_DATE_CODE                   "20260208"
#define APP_SW_BUILD_ID                 "1.0.0"
#define APP_HW_VERSION                  1
#define APP_ZCL_VERSION                 3

// Timing Configuration
#define APP_SENSOR_READ_PERIOD_MS       10000   // 10 seconds
#define APP_FAST_POLL_TIMEOUT_MS        30000   // 30 seconds fast poll after join
#define APP_FAST_POLL_INTERVAL_QS       2       // 200ms (in quarter seconds)
#define APP_NORMAL_POLL_INTERVAL_QS     30      // 7.5 seconds (in quarter seconds)

// Battery voltage range (2xAA: 2.0V - 3.2V)
#define BATTERY_VOLTAGE_MIN_MV          2000
#define BATTERY_VOLTAGE_MAX_MV          3200
#define BATTERY_VOLTAGE_NOMINAL_MV      3000

//==============================================================================
// Application State
//==============================================================================

typedef enum {
  APP_STATE_INIT,
  APP_STATE_NOT_JOINED,
  APP_STATE_JOINING,
  APP_STATE_JOINED_FAST_POLL,
  APP_STATE_JOINED_NORMAL,
  APP_STATE_LEAVING
} AppState_t;

typedef struct {
  AppState_t state;
  bool fastPollActive;
  uint32_t joinTimestamp;
  uint8_t joinAttempts;
  bool sensorInitialized;
  bool buttonPressed;
} AppContext_t;

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Application initialization
 * Called once at startup after stack init
 */
void app_init(void);

/**
 * @brief Application process loop
 * Called periodically by the framework
 */
void app_process_action(void);

/**
 * @brief Get current application state
 * @return Current AppState_t
 */
AppState_t app_get_state(void);

/**
 * @brief Trigger immediate sensor reading and attribute update
 */
void app_trigger_sensor_read(void);

/**
 * @brief Start network join process
 */
void app_start_join(void);

/**
 * @brief Leave network
 */
void app_leave_network(void);

/**
 * @brief Network event handler
 * @param networkEvent Network event type
 */
void app_network_event_handler(EmberAfNetworkEvent networkEvent);

/**
 * @brief Stack status callback
 * @param status Stack status
 */
void app_stack_status_callback(EmberStatus status);

/**
 * @brief Enable/disable fast poll mode
 * @param enable true to enable fast poll, false for normal poll
 */
void app_set_fast_poll(bool enable);

/**
 * @brief Update sensor measurements and attributes
 */
void app_update_sensor_data(void);

/**
 * @brief Update battery measurements and attributes
 */
void app_update_battery_data(void);

//==============================================================================
// CLI Command Handlers
//==============================================================================

void cli_sensor_read(sl_cli_command_arg_t *arguments);
void cli_battery_read(sl_cli_command_arg_t *arguments);
void cli_network_status(sl_cli_command_arg_t *arguments);

//==============================================================================
// Logging Macros
//==============================================================================

#define APP_LOG(...)    emberAfCorePrintln(__VA_ARGS__)
#define APP_INFO(...)   emberAfCorePrintln(__VA_ARGS__)
#define APP_ERROR(...)  emberAfCorePrintln("ERROR: " __VA_ARGS__)
#define APP_DEBUG(...)  emberAfDebugPrintln(__VA_ARGS__)

#endif // APP_H
