/**
 * @file battery.c
 * @brief Battery voltage monitoring implementation
 *
 * Uses ADC to measure AVDD (battery voltage) and converts to percentage
 */

#include "battery.h"
#include "app.h"
#include "em_adc.h"
#include "em_cmu.h"

//==============================================================================
// Private Variables
//==============================================================================

// ADC resolution (12-bit)
#define ADC_RESOLUTION      4096

// AVDD reference voltage (mV) - typically 3.3V for internal reference
// We'll measure AVDD relative to internal 1.25V reference
#define ADC_REF_VOLTAGE_MV  1250

//==============================================================================
// Public Functions
//==============================================================================

void battery_init(void)
{
  APP_LOG("Initializing battery monitor...");

  // Enable ADC clock
  CMU_ClockEnable(cmuClock_ADC0, true);

  // Initialize ADC for single conversion
  ADC_Init_TypeDef adcInit = ADC_INIT_DEFAULT;
  adcInit.timebase = ADC_TimebaseCalc(0);
  adcInit.prescale = ADC_PrescaleCalc(1000000, 0);  // 1 MHz ADC clock
  ADC_Init(ADC0, &adcInit);

  // Configure ADC for AVDD measurement
  ADC_InitSingle_TypeDef singleInit = ADC_INITSINGLE_DEFAULT;
  singleInit.input = adcSingleInputAVDD;         // Measure AVDD
  singleInit.reference = adcRef1V25;             // 1.25V internal reference
  singleInit.resolution = adcRes12Bit;           // 12-bit resolution
  singleInit.acqTime = adcAcqTime256;            // Longer acquisition for stability
  ADC_InitSingle(ADC0, &singleInit);

  APP_LOG("Battery monitor initialized (ADC0, AVDD measurement)");
}

uint16_t battery_read_voltage(void)
{
  // Start ADC conversion
  ADC_Start(ADC0, adcStartSingle);

  // Wait for conversion to complete
  while (ADC0->STATUS & ADC_STATUS_SINGLEACT);

  // Read ADC result
  uint32_t adcResult = ADC_DataSingleGet(ADC0);

  // Calculate voltage in mV
  // AVDD = (ADC_result * REF_voltage * scale) / ADC_max
  // For AVDD input, scale factor is 3 (AVDD is divided by 3 internally)
  uint32_t voltage_mv = (adcResult * ADC_REF_VOLTAGE_MV * 3) / ADC_RESOLUTION;

  APP_DEBUG("ADC: raw=%lu, voltage=%lu mV", adcResult, voltage_mv);

  return (uint16_t)voltage_mv;
}

uint8_t battery_voltage_to_percentage(uint16_t voltage_mv)
{
  // 2xAA battery range: 2.0V (empty) to 3.2V (full)
  // Linear approximation (good enough for alkaline AA batteries)

  if (voltage_mv >= BATTERY_VOLTAGE_MAX_MV) {
    return 100;
  }

  if (voltage_mv <= BATTERY_VOLTAGE_MIN_MV) {
    return 0;
  }

  // Linear interpolation
  uint32_t range = BATTERY_VOLTAGE_MAX_MV - BATTERY_VOLTAGE_MIN_MV;
  uint32_t delta = voltage_mv - BATTERY_VOLTAGE_MIN_MV;
  uint8_t percentage = (uint8_t)((delta * 100) / range);

  return percentage;
}
