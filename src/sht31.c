/**
 * @file sht31.c
 * @brief SHT31 sensor driver implementation
 *
 * Implements I2C communication with SHT31 sensor and fallback mode
 */

#include "sht31.h"
#include "app.h"
#include "em_i2c.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "sl_sleeptimer.h"
#include <math.h>

//==============================================================================
// Private Variables
//==============================================================================

static bool sensorPresent = false;
static uint32_t fallbackReadCount = 0;

//==============================================================================
// Forward Declarations
//==============================================================================

static bool i2c_write_command(uint8_t cmd_msb, uint8_t cmd_lsb);
static bool i2c_read_data(uint8_t *data, uint8_t len);
static uint8_t calculate_crc(const uint8_t *data, uint8_t len);
static void generate_fallback_values(float *temperature_c, float *humidity_rh);
static void delay_ms(uint32_t ms);

//==============================================================================
// Public Functions
//==============================================================================

bool sht31_init(void)
{
  APP_LOG("Initializing SHT31 sensor...");

  // Enable clocks
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(cmuClock_I2C0, true);

  // Configure GPIO pins for I2C
  GPIO_PinModeSet(SHT31_SDA_PORT, SHT31_SDA_PIN, gpioModeWiredAndPullUpFilter, 1);
  GPIO_PinModeSet(SHT31_SCL_PORT, SHT31_SCL_PIN, gpioModeWiredAndPullUpFilter, 1);

  // Initialize I2C
  I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;
  i2cInit.freq = I2C_FREQ_STANDARD_MAX;  // 100 kHz
  i2cInit.clhr = i2cClockHLRStandard;

  // Route I2C pins
  I2C0->ROUTEPEN = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
  I2C0->ROUTELOC0 = (SHT31_SDA_LOC) | (SHT31_SCL_LOC);

  I2C_Init(I2C0, &i2cInit);

  // Small delay for sensor power-up
  delay_ms(10);

  // Try to reset sensor to check if present
  sensorPresent = sht31_reset();

  if (sensorPresent) {
    APP_LOG("SHT31 sensor detected at address 0x%02X", SHT31_I2C_ADDR);
  } else {
    APP_LOG("SHT31 sensor NOT detected - using fallback mode");
  }

  return sensorPresent;
}

bool sht31_read(float *temperature_c, float *humidity_rh)
{
  if (!sensorPresent) {
    // Use fallback values
    generate_fallback_values(temperature_c, humidity_rh);
    return false;
  }

  // Send measurement command (high repeatability)
  if (!i2c_write_command(SHT31_CMD_READ_MSB, SHT31_CMD_READ_LSB)) {
    APP_ERROR("Failed to send measurement command");
    sensorPresent = false;
    generate_fallback_values(temperature_c, humidity_rh);
    return false;
  }

  // Wait for measurement to complete
  delay_ms(SHT31_MEASURE_DELAY_MS);

  // Read 6 bytes: temp_msb, temp_lsb, temp_crc, hum_msb, hum_lsb, hum_crc
  uint8_t data[6];
  if (!i2c_read_data(data, 6)) {
    APP_ERROR("Failed to read measurement data");
    sensorPresent = false;
    generate_fallback_values(temperature_c, humidity_rh);
    return false;
  }

  // Verify CRC for temperature
  uint8_t temp_crc = calculate_crc(&data[0], 2);
  if (temp_crc != data[2]) {
    APP_ERROR("Temperature CRC mismatch: expected 0x%02X, got 0x%02X", temp_crc, data[2]);
    generate_fallback_values(temperature_c, humidity_rh);
    return false;
  }

  // Verify CRC for humidity
  uint8_t hum_crc = calculate_crc(&data[3], 2);
  if (hum_crc != data[5]) {
    APP_ERROR("Humidity CRC mismatch: expected 0x%02X, got 0x%02X", hum_crc, data[5]);
    generate_fallback_values(temperature_c, humidity_rh);
    return false;
  }

  // Convert temperature (formula from datasheet)
  uint16_t temp_raw = (data[0] << 8) | data[1];
  *temperature_c = -45.0f + (175.0f * temp_raw / 65535.0f);

  // Convert humidity (formula from datasheet)
  uint16_t hum_raw = (data[3] << 8) | data[4];
  *humidity_rh = 100.0f * hum_raw / 65535.0f;

  // Clamp humidity to valid range
  if (*humidity_rh < 0.0f) *humidity_rh = 0.0f;
  if (*humidity_rh > 100.0f) *humidity_rh = 100.0f;

  return true;
}

bool sht31_reset(void)
{
  return i2c_write_command(SHT31_CMD_SOFT_RESET_MSB, SHT31_CMD_SOFT_RESET_LSB);
}

bool sht31_is_present(void)
{
  return sensorPresent;
}

//==============================================================================
// Private Functions
//==============================================================================

/**
 * @brief Write 2-byte command to SHT31
 */
static bool i2c_write_command(uint8_t cmd_msb, uint8_t cmd_lsb)
{
  I2C_TransferSeq_TypeDef seq;
  uint8_t cmd[2] = {cmd_msb, cmd_lsb};

  seq.addr = SHT31_I2C_ADDR << 1;
  seq.flags = I2C_FLAG_WRITE;
  seq.buf[0].data = cmd;
  seq.buf[0].len = 2;

  I2C_TransferReturn_TypeDef ret = I2C_TransferInit(I2C0, &seq);

  while (ret == i2cTransferInProgress) {
    ret = I2C_Transfer(I2C0);
  }

  return (ret == i2cTransferDone);
}

/**
 * @brief Read data from SHT31
 */
static bool i2c_read_data(uint8_t *data, uint8_t len)
{
  I2C_TransferSeq_TypeDef seq;

  seq.addr = SHT31_I2C_ADDR << 1;
  seq.flags = I2C_FLAG_READ;
  seq.buf[0].data = data;
  seq.buf[0].len = len;

  I2C_TransferReturn_TypeDef ret = I2C_TransferInit(I2C0, &seq);

  while (ret == i2cTransferInProgress) {
    ret = I2C_Transfer(I2C0);
  }

  return (ret == i2cTransferDone);
}

/**
 * @brief Calculate CRC-8 for SHT31 data
 * Polynomial: 0x31 (x^8 + x^5 + x^4 + 1)
 */
static uint8_t calculate_crc(const uint8_t *data, uint8_t len)
{
  uint8_t crc = 0xFF;  // Initial value

  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];

    for (uint8_t bit = 0; bit < 8; bit++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31;
      } else {
        crc = crc << 1;
      }
    }
  }

  return crc;
}

/**
 * @brief Generate realistic fallback sensor values
 * Uses slow drift pattern for testing when sensor is not present
 */
static void generate_fallback_values(float *temperature_c, float *humidity_rh)
{
  fallbackReadCount++;

  // Generate slowly drifting fake values for realistic testing
  // Temperature: 20-25°C with slow sine wave
  float temp_base = 22.5f;
  float temp_variation = 2.5f * sinf(fallbackReadCount * 0.1f);
  *temperature_c = temp_base + temp_variation;

  // Humidity: 40-60% with different phase
  float hum_base = 50.0f;
  float hum_variation = 10.0f * sinf(fallbackReadCount * 0.15f + 1.57f);
  *humidity_rh = hum_base + hum_variation;

  APP_DEBUG("Fallback values: temp=%.2f°C, humidity=%.2f%% (count=%lu)",
            *temperature_c, *humidity_rh, fallbackReadCount);
}

/**
 * @brief Simple millisecond delay
 */
static void delay_ms(uint32_t ms)
{
  sl_sleeptimer_delay_millisecond(ms);
}
