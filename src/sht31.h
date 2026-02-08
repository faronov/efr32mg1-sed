/**
 * @file sht31.h
 * @brief SHT31 temperature and humidity sensor driver
 *
 * I2C driver for Sensirion SHT31 sensor with fallback mode
 */

#ifndef SHT31_H
#define SHT31_H

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// Configuration
//==============================================================================

// I2C Configuration
#define SHT31_I2C_PORT      0           // I2C0
#define SHT31_SDA_PORT      gpioPortC
#define SHT31_SDA_PIN       10          // PC10 (Pin 24)
#define SHT31_SCL_PORT      gpioPortC
#define SHT31_SCL_PIN       11          // PC11 (Pin 25)
#define SHT31_SDA_LOC       _I2C_ROUTELOC0_SDALOC_LOC14
#define SHT31_SCL_LOC       _I2C_ROUTELOC0_SCLLOC_LOC14

// SHT31 I2C Address
#define SHT31_I2C_ADDR      0x44        // Default address (ADDR pin to GND)

// SHT31 Commands
#define SHT31_CMD_READ_MSB  0x24        // Measurement: high repeatability
#define SHT31_CMD_READ_LSB  0x00
#define SHT31_CMD_SOFT_RESET_MSB 0x30
#define SHT31_CMD_SOFT_RESET_LSB 0xA2
#define SHT31_CMD_STATUS_MSB 0xF3
#define SHT31_CMD_STATUS_LSB 0x2D

// Timing
#define SHT31_MEASURE_DELAY_MS  20      // Measurement duration

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Initialize SHT31 sensor
 * Configures I2C and attempts to communicate with sensor
 *
 * @return true if sensor detected, false if using fallback mode
 */
bool sht31_init(void);

/**
 * @brief Read temperature and humidity from SHT31
 *
 * @param[out] temperature_c Temperature in degrees Celsius
 * @param[out] humidity_rh Relative humidity in percent
 * @return true if successful (real sensor), false if using fallback values
 */
bool sht31_read(float *temperature_c, float *humidity_rh);

/**
 * @brief Reset SHT31 sensor
 * @return true if successful
 */
bool sht31_reset(void);

/**
 * @brief Check if sensor is present
 * @return true if real sensor is connected
 */
bool sht31_is_present(void);

#endif // SHT31_H
