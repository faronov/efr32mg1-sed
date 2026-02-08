/**
 * @file zcl_callbacks.c
 * @brief ZCL cluster callback implementations
 *
 * Handles cluster-specific callbacks for Basic, Identify, Power Config,
 * Temperature Measurement, and Relative Humidity Measurement clusters.
 */

#include "app.h"
#include "af.h"
#include "app/framework/include/af.h"
#include "sl_component_catalog.h"

#ifdef SL_CATALOG_SIMPLE_LED_PRESENT
#include "sl_simple_led_instances.h"
#endif

//==============================================================================
// Private Variables
//==============================================================================

static bool identifyActive = false;
static sl_sleeptimer_timer_handle_t identifyTimer;
static uint16_t identifyTimeRemaining = 0;

//==============================================================================
// Forward Declarations
//==============================================================================

static void identify_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);
static void identify_led_blink(void);

//==============================================================================
// Basic Cluster Callbacks
//==============================================================================

/**
 * @brief Basic cluster init callback
 * Set manufacturer-specific attributes
 */
void emberAfBasicClusterServerInitCallback(uint8_t endpoint)
{
  APP_LOG("Basic cluster init for endpoint %d", endpoint);

  // Set manufacturer name
  emberAfWriteServerAttribute(endpoint,
                               ZCL_BASIC_CLUSTER_ID,
                               ZCL_MANUFACTURER_NAME_ATTRIBUTE_ID,
                               (uint8_t*)APP_MANUFACTURER_NAME,
                               ZCL_CHAR_STRING_ATTRIBUTE_TYPE);

  // Set model identifier
  emberAfWriteServerAttribute(endpoint,
                               ZCL_BASIC_CLUSTER_ID,
                               ZCL_MODEL_IDENTIFIER_ATTRIBUTE_ID,
                               (uint8_t*)APP_MODEL_IDENTIFIER,
                               ZCL_CHAR_STRING_ATTRIBUTE_TYPE);

  // Set date code
  emberAfWriteServerAttribute(endpoint,
                               ZCL_BASIC_CLUSTER_ID,
                               ZCL_DATE_CODE_ATTRIBUTE_ID,
                               (uint8_t*)APP_DATE_CODE,
                               ZCL_CHAR_STRING_ATTRIBUTE_TYPE);

  // Set software build ID
  emberAfWriteServerAttribute(endpoint,
                               ZCL_BASIC_CLUSTER_ID,
                               ZCL_SW_BUILD_ID_ATTRIBUTE_ID,
                               (uint8_t*)APP_SW_BUILD_ID,
                               ZCL_CHAR_STRING_ATTRIBUTE_TYPE);

  // Set hardware version
  uint8_t hwVersion = APP_HW_VERSION;
  emberAfWriteServerAttribute(endpoint,
                               ZCL_BASIC_CLUSTER_ID,
                               ZCL_HW_VERSION_ATTRIBUTE_ID,
                               &hwVersion,
                               ZCL_INT8U_ATTRIBUTE_TYPE);

  // Set ZCL version
  uint8_t zclVersion = APP_ZCL_VERSION;
  emberAfWriteServerAttribute(endpoint,
                               ZCL_BASIC_CLUSTER_ID,
                               ZCL_ZCL_VERSION_ATTRIBUTE_ID,
                               &zclVersion,
                               ZCL_INT8U_ATTRIBUTE_TYPE);

  // Set power source (battery)
  uint8_t powerSource = EMBER_ZCL_POWER_SOURCE_SINGLE_PHASE_MAINS;  // 0x01
  emberAfWriteServerAttribute(endpoint,
                               ZCL_BASIC_CLUSTER_ID,
                               ZCL_POWER_SOURCE_ATTRIBUTE_ID,
                               &powerSource,
                               ZCL_ENUM8_ATTRIBUTE_TYPE);
}

/**
 * @brief Reset to factory defaults callback
 */
bool emberAfBasicClusterResetToFactoryDefaultsCallback(void)
{
  APP_LOG("Reset to factory defaults requested");

  // Leave network and clear all settings
  if (emberAfNetworkState() == EMBER_JOINED_NETWORK) {
    emberLeaveNetwork();
  }

  // Clear tokens/NVM (if needed)
  // emberEraseKeyTableEntry(0xFF);  // Erase all keys

  // Send default response
  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return true;
}

//==============================================================================
// Identify Cluster Callbacks
//==============================================================================

/**
 * @brief Identify cluster init callback
 */
void emberAfIdentifyClusterServerInitCallback(uint8_t endpoint)
{
  APP_LOG("Identify cluster init for endpoint %d", endpoint);

  // Initialize identify time to 0
  uint16_t identifyTime = 0;
  emberAfWriteServerAttribute(endpoint,
                               ZCL_IDENTIFY_CLUSTER_ID,
                               ZCL_IDENTIFY_TIME_ATTRIBUTE_ID,
                               (uint8_t*)&identifyTime,
                               ZCL_INT16U_ATTRIBUTE_TYPE);
}

/**
 * @brief Identify command callback
 * Start visual identification (LED blinking)
 */
bool emberAfIdentifyClusterIdentifyCallback(uint16_t identifyTime)
{
  APP_LOG("Identify command received: time=%d seconds", identifyTime);

  identifyTimeRemaining = identifyTime;

  if (identifyTime > 0) {
    if (!identifyActive) {
      identifyActive = true;
      // Start 1-second periodic timer for LED blinking
      sl_sleeptimer_start_periodic_timer_ms(&identifyTimer,
                                             1000,
                                             identify_timer_callback,
                                             NULL,
                                             0,
                                             0);
      APP_LOG("Identify started");
    }
  } else {
    // Stop identify
    if (identifyActive) {
      identifyActive = false;
      sl_sleeptimer_stop_timer(&identifyTimer);
#ifdef SL_CATALOG_SIMPLE_LED_PRESENT
      sl_led_turn_off(&sl_led_led0);
#endif
      APP_LOG("Identify stopped");
    }
  }

  // Update attribute
  emberAfWriteServerAttribute(APP_ENDPOINT,
                               ZCL_IDENTIFY_CLUSTER_ID,
                               ZCL_IDENTIFY_TIME_ATTRIBUTE_ID,
                               (uint8_t*)&identifyTimeRemaining,
                               ZCL_INT16U_ATTRIBUTE_TYPE);

  emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_SUCCESS);
  return true;
}

/**
 * @brief Identify query command callback
 */
bool emberAfIdentifyClusterIdentifyQueryCallback(void)
{
  APP_LOG("Identify query received");

  // Send identify query response
  emberAfFillCommandIdentifyClusterIdentifyQueryResponse(identifyTimeRemaining);
  emberAfSendResponse();
  return true;
}

/**
 * @brief Identify timer callback
 * Called every second during identify
 */
static void identify_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;

  if (identifyTimeRemaining > 0) {
    identifyTimeRemaining--;

    // Blink LED
    identify_led_blink();

    // Update attribute
    emberAfWriteServerAttribute(APP_ENDPOINT,
                                 ZCL_IDENTIFY_CLUSTER_ID,
                                 ZCL_IDENTIFY_TIME_ATTRIBUTE_ID,
                                 (uint8_t*)&identifyTimeRemaining,
                                 ZCL_INT16U_ATTRIBUTE_TYPE);

    if (identifyTimeRemaining == 0) {
      // Identify finished
      identifyActive = false;
      sl_sleeptimer_stop_timer(&identifyTimer);
#ifdef SL_CATALOG_SIMPLE_LED_PRESENT
      sl_led_turn_off(&sl_led_led0);
#endif
      APP_LOG("Identify finished");
    }
  }
}

/**
 * @brief Blink LED for identify
 */
static void identify_led_blink(void)
{
#ifdef SL_CATALOG_SIMPLE_LED_PRESENT
  sl_led_toggle(&sl_led_led0);
#endif
}

//==============================================================================
// Power Configuration Cluster Callbacks
//==============================================================================

/**
 * @brief Power configuration cluster init callback
 */
void emberAfPowerConfigClusterServerInitCallback(uint8_t endpoint)
{
  APP_LOG("Power Config cluster init for endpoint %d", endpoint);

  // Set battery information
  uint8_t batteryQuantity = 2;  // 2xAA
  emberAfWriteServerAttribute(endpoint,
                               ZCL_POWER_CONFIG_CLUSTER_ID,
                               ZCL_BATTERY_QUANTITY_ATTRIBUTE_ID,
                               &batteryQuantity,
                               ZCL_INT8U_ATTRIBUTE_TYPE);

  // Set battery size
  uint8_t batterySize = EMBER_ZCL_BATTERY_SIZE_AA;  // 0x03
  emberAfWriteServerAttribute(endpoint,
                               ZCL_POWER_CONFIG_CLUSTER_ID,
                               ZCL_BATTERY_SIZE_ATTRIBUTE_ID,
                               &batterySize,
                               ZCL_ENUM8_ATTRIBUTE_TYPE);

  // Set battery voltage min/max (in 100mV units)
  uint8_t batteryVoltageMin = BATTERY_VOLTAGE_MIN_MV / 100;
  uint8_t batteryVoltageMax = BATTERY_VOLTAGE_MAX_MV / 100;

  emberAfWriteServerAttribute(endpoint,
                               ZCL_POWER_CONFIG_CLUSTER_ID,
                               ZCL_BATTERY_VOLTAGE_MIN_THRESHOLD_ATTRIBUTE_ID,
                               &batteryVoltageMin,
                               ZCL_INT8U_ATTRIBUTE_TYPE);

  emberAfWriteServerAttribute(endpoint,
                               ZCL_POWER_CONFIG_CLUSTER_ID,
                               ZCL_BATTERY_VOLTAGE_THRESHOLD1_ATTRIBUTE_ID,
                               &batteryVoltageMax,
                               ZCL_INT8U_ATTRIBUTE_TYPE);

  // Set battery alarm state (no alarm)
  uint8_t batteryAlarmState = 0;
  emberAfWriteServerAttribute(endpoint,
                               ZCL_POWER_CONFIG_CLUSTER_ID,
                               ZCL_BATTERY_ALARM_STATE_ATTRIBUTE_ID,
                               &batteryAlarmState,
                               ZCL_BITMAP8_ATTRIBUTE_TYPE);
}

//==============================================================================
// Temperature Measurement Cluster Callbacks
//==============================================================================

/**
 * @brief Temperature measurement cluster init callback
 */
void emberAfTempMeasurementClusterServerInitCallback(uint8_t endpoint)
{
  APP_LOG("Temperature Measurement cluster init for endpoint %d", endpoint);

  // Set min/max measured values (in 0.01°C units)
  int16_t minValue = -4000;  // -40°C
  int16_t maxValue = 12500;  // 125°C

  emberAfWriteServerAttribute(endpoint,
                               ZCL_TEMP_MEASUREMENT_CLUSTER_ID,
                               ZCL_TEMP_MIN_MEASURED_VALUE_ATTRIBUTE_ID,
                               (uint8_t*)&minValue,
                               ZCL_INT16S_ATTRIBUTE_TYPE);

  emberAfWriteServerAttribute(endpoint,
                               ZCL_TEMP_MEASUREMENT_CLUSTER_ID,
                               ZCL_TEMP_MAX_MEASURED_VALUE_ATTRIBUTE_ID,
                               (uint8_t*)&maxValue,
                               ZCL_INT16S_ATTRIBUTE_TYPE);

  // Set tolerance (±0.3°C = 30 in 0.01°C units)
  uint16_t tolerance = 30;
  emberAfWriteServerAttribute(endpoint,
                               ZCL_TEMP_MEASUREMENT_CLUSTER_ID,
                               ZCL_TEMP_TOLERANCE_ATTRIBUTE_ID,
                               (uint8_t*)&tolerance,
                               ZCL_INT16U_ATTRIBUTE_TYPE);
}

//==============================================================================
// Relative Humidity Measurement Cluster Callbacks
//==============================================================================

/**
 * @brief Relative humidity measurement cluster init callback
 */
void emberAfRelativeHumidityMeasurementClusterServerInitCallback(uint8_t endpoint)
{
  APP_LOG("Relative Humidity Measurement cluster init for endpoint %d", endpoint);

  // Set min/max measured values (in 0.01% units)
  uint16_t minValue = 0;      // 0%
  uint16_t maxValue = 10000;  // 100%

  emberAfWriteServerAttribute(endpoint,
                               ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID,
                               ZCL_RELATIVE_HUMIDITY_MIN_MEASURED_VALUE_ATTRIBUTE_ID,
                               (uint8_t*)&minValue,
                               ZCL_INT16U_ATTRIBUTE_TYPE);

  emberAfWriteServerAttribute(endpoint,
                               ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID,
                               ZCL_RELATIVE_HUMIDITY_MAX_MEASURED_VALUE_ATTRIBUTE_ID,
                               (uint8_t*)&maxValue,
                               ZCL_INT16U_ATTRIBUTE_TYPE);

  // Set tolerance (±2% = 200 in 0.01% units)
  uint16_t tolerance = 200;
  emberAfWriteServerAttribute(endpoint,
                               ZCL_RELATIVE_HUMIDITY_MEASUREMENT_CLUSTER_ID,
                               ZCL_RELATIVE_HUMIDITY_TOLERANCE_ATTRIBUTE_ID,
                               (uint8_t*)&tolerance,
                               ZCL_INT16U_ATTRIBUTE_TYPE);
}

//==============================================================================
// Reporting Callbacks
//==============================================================================

/**
 * @brief Configure reporting command callback
 * Called when coordinator configures attribute reporting
 */
bool emberAfConfigureReportingCommandCallback(const EmberAfClusterCommand *cmd)
{
  APP_LOG("Configure reporting command received");
  APP_LOG("  Cluster: 0x%04X", cmd->apsFrame->clusterId);
  APP_LOG("  Endpoint: %d", cmd->apsFrame->destinationEndpoint);

  // Let the framework handle it
  return false;  // false = framework continues processing
}

/**
 * @brief Read reporting configuration command callback
 */
bool emberAfReadReportingConfigurationCommandCallback(const EmberAfClusterCommand *cmd)
{
  APP_LOG("Read reporting configuration command received");
  APP_LOG("  Cluster: 0x%04X", cmd->apsFrame->clusterId);

  // Let the framework handle it
  return false;
}

//==============================================================================
// General ZCL Callbacks
//==============================================================================

/**
 * @brief Pre-command received callback
 * Called before processing any ZCL command
 */
bool emberAfPreCommandReceivedCallback(EmberAfClusterCommand *cmd)
{
  // Log all incoming commands for debugging
  APP_DEBUG("ZCL command received:");
  APP_DEBUG("  Cluster: 0x%04X", cmd->apsFrame->clusterId);
  APP_DEBUG("  Command: 0x%02X", cmd->commandId);
  APP_DEBUG("  Endpoint: %d", cmd->apsFrame->destinationEndpoint);

  // Allow framework to continue processing
  return false;
}

/**
 * @brief Pre-attribute change callback
 * Called before any attribute is changed
 */
EmberAfStatus emberAfPreAttributeChangeCallback(uint8_t endpoint,
                                                  EmberAfClusterId clusterId,
                                                  EmberAfAttributeId attributeId,
                                                  uint8_t mask,
                                                  uint16_t manufacturerCode,
                                                  uint8_t type,
                                                  uint8_t size,
                                                  uint8_t *value)
{
  APP_DEBUG("Attribute change: EP=%d, cluster=0x%04X, attr=0x%04X",
            endpoint, clusterId, attributeId);

  // Allow all attribute changes
  return EMBER_ZCL_STATUS_SUCCESS;
}

/**
 * @brief Post-attribute change callback
 * Called after any attribute is changed
 */
void emberAfPostAttributeChangeCallback(uint8_t endpoint,
                                         EmberAfClusterId clusterId,
                                         EmberAfAttributeId attributeId,
                                         uint8_t mask,
                                         uint16_t manufacturerCode,
                                         uint8_t type,
                                         uint8_t size,
                                         uint8_t *value)
{
  APP_DEBUG("Attribute changed: EP=%d, cluster=0x%04X, attr=0x%04X",
            endpoint, clusterId, attributeId);
}

/**
 * @brief Default response callback
 * Called when a default response is received
 */
bool emberAfDefaultResponseCallback(EmberAfClusterId clusterId,
                                     uint8_t commandId,
                                     EmberAfStatus status)
{
  APP_DEBUG("Default response: cluster=0x%04X, cmd=0x%02X, status=0x%02X",
            clusterId, commandId, status);

  return false;
}
