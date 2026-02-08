/**
 * @file battery.h
 * @brief Battery voltage monitoring via ADC
 *
 * Monitors battery voltage (2xAA) and converts to Zigbee battery attributes
 */

#ifndef BATTERY_H
#define BATTERY_H

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Initialize battery monitoring
 * Configures ADC for AVDD measurement
 */
void battery_init(void);

/**
 * @brief Read battery voltage
 * @return Voltage in millivolts
 */
uint16_t battery_read_voltage(void);

/**
 * @brief Convert voltage to percentage
 * For 2xAA batteries (2.0V - 3.2V range)
 *
 * @param voltage_mv Voltage in millivolts
 * @return Battery percentage (0-100)
 */
uint8_t battery_voltage_to_percentage(uint16_t voltage_mv);

#endif // BATTERY_H
