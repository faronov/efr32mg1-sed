/**
 * @file button.c
 * @brief Button driver implementation
 *
 * Implements robust button handling with debouncing and press type detection.
 */

#include "button.h"
#include "app.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "gpiointerrupt.h"
#include "sl_sleeptimer.h"

//==============================================================================
// Private Types
//==============================================================================

typedef enum {
  BUTTON_STATE_IDLE,
  BUTTON_STATE_DEBOUNCE_PRESS,
  BUTTON_STATE_PRESSED,
  BUTTON_STATE_DEBOUNCE_RELEASE,
  BUTTON_STATE_LONG_PRESS_TRIGGERED
} ButtonState_t;

typedef struct {
  ButtonState_t state;
  uint32_t pressTimestamp;
  uint32_t releaseTimestamp;
  bool interruptPending;
  bool longPressTriggered;
} ButtonContext_t;

//==============================================================================
// Private Variables
//==============================================================================

static ButtonContext_t buttonContext = {
  .state = BUTTON_STATE_IDLE,
  .pressTimestamp = 0,
  .releaseTimestamp = 0,
  .interruptPending = false,
  .longPressTriggered = false
};

//==============================================================================
// Forward Declarations
//==============================================================================

static void button_gpio_callback(uint8_t intNo);
static bool is_button_physically_pressed(void);
static uint32_t get_time_ms(void);

//==============================================================================
// Public Functions
//==============================================================================

void button_init(void)
{
  // Enable GPIO clock
  CMU_ClockEnable(cmuClock_GPIO, true);

  // Configure PB13 as input with pull-up (button is active-low)
  GPIO_PinModeSet(BUTTON_PORT, BUTTON_PIN, gpioModeInputPullFilter, 1);

  // Initialize GPIO interrupt dispatcher
  GPIOINT_Init();

  // Register callback for PB13
  GPIOINT_CallbackRegister(BUTTON_PIN, button_gpio_callback);

  // Enable interrupt on both edges (press and release)
  GPIO_ExtIntConfig(BUTTON_PORT, BUTTON_PIN, BUTTON_PIN, true, true, true);

  APP_LOG("Button initialized: port=%c, pin=%d (active-low with pull-up)",
          'A' + BUTTON_PORT, BUTTON_PIN);
}

void button_process(void)
{
  uint32_t currentTime = get_time_ms();
  uint32_t pressedDuration;

  switch (buttonContext.state) {
    case BUTTON_STATE_IDLE:
      // Check for interrupt
      if (buttonContext.interruptPending) {
        buttonContext.interruptPending = false;

        if (is_button_physically_pressed()) {
          // Button pressed - start debounce
          buttonContext.state = BUTTON_STATE_DEBOUNCE_PRESS;
          buttonContext.pressTimestamp = currentTime;
          APP_DEBUG("Button interrupt: PRESS detected, starting debounce");
        }
      }
      break;

    case BUTTON_STATE_DEBOUNCE_PRESS:
      // Wait for debounce time
      if (currentTime - buttonContext.pressTimestamp >= BUTTON_DEBOUNCE_MS) {
        // Check if still pressed after debounce
        if (is_button_physically_pressed()) {
          // Valid press confirmed
          buttonContext.state = BUTTON_STATE_PRESSED;
          buttonContext.longPressTriggered = false;
          APP_DEBUG("Button press confirmed (debounced)");
        } else {
          // False trigger - return to idle
          buttonContext.state = BUTTON_STATE_IDLE;
          APP_DEBUG("Button press rejected (debounce failed)");
        }
      }
      break;

    case BUTTON_STATE_PRESSED:
      // Check for release interrupt
      if (buttonContext.interruptPending) {
        buttonContext.interruptPending = false;

        if (!is_button_physically_pressed()) {
          // Button released - start debounce
          buttonContext.state = BUTTON_STATE_DEBOUNCE_RELEASE;
          buttonContext.releaseTimestamp = currentTime;
          APP_DEBUG("Button interrupt: RELEASE detected, starting debounce");
        }
      }

      // Check for long press
      pressedDuration = currentTime - buttonContext.pressTimestamp;
      if (!buttonContext.longPressTriggered &&
          pressedDuration >= BUTTON_LONG_PRESS_MS) {
        // Long press detected
        buttonContext.longPressTriggered = true;
        buttonContext.state = BUTTON_STATE_LONG_PRESS_TRIGGERED;
        APP_LOG("Button LONG PRESS detected (%lu ms)", pressedDuration);
        button_long_press_callback();
      }
      break;

    case BUTTON_STATE_DEBOUNCE_RELEASE:
      // Wait for debounce time
      if (currentTime - buttonContext.releaseTimestamp >= BUTTON_DEBOUNCE_MS) {
        // Check if still released after debounce
        if (!is_button_physically_pressed()) {
          // Valid release confirmed
          pressedDuration = buttonContext.releaseTimestamp - buttonContext.pressTimestamp;

          if (!buttonContext.longPressTriggered) {
            // Short press
            APP_LOG("Button SHORT PRESS detected (%lu ms)", pressedDuration);
            button_short_press_callback();
          } else {
            APP_DEBUG("Button released after long press");
          }

          buttonContext.state = BUTTON_STATE_IDLE;
        } else {
          // False trigger - button still pressed
          buttonContext.state = BUTTON_STATE_PRESSED;
          APP_DEBUG("Button release rejected (debounce failed)");
        }
      }
      break;

    case BUTTON_STATE_LONG_PRESS_TRIGGERED:
      // Wait for button release
      if (buttonContext.interruptPending) {
        buttonContext.interruptPending = false;

        if (!is_button_physically_pressed()) {
          // Button released
          buttonContext.state = BUTTON_STATE_DEBOUNCE_RELEASE;
          buttonContext.releaseTimestamp = currentTime;
          APP_DEBUG("Button released after long press");
        }
      }
      break;
  }
}

bool button_is_pressed(void)
{
  return is_button_physically_pressed();
}

//==============================================================================
// Private Functions
//==============================================================================

/**
 * @brief GPIO interrupt callback
 * Called by GPIO interrupt handler on both edges
 */
static void button_gpio_callback(uint8_t intNo)
{
  (void)intNo;

  // Set flag for processing in main loop
  buttonContext.interruptPending = true;
}

/**
 * @brief Read physical button state
 * @return true if button is pressed (pin is LOW, active-low)
 */
static bool is_button_physically_pressed(void)
{
  // Button is active-low with pull-up
  return (GPIO_PinInGet(BUTTON_PORT, BUTTON_PIN) == 0);
}

/**
 * @brief Get current time in milliseconds
 * @return Current tick count in ms
 */
static uint32_t get_time_ms(void)
{
  // Use sleeptimer for consistent timing
  uint64_t ticks = sl_sleeptimer_get_tick_count64();
  return (uint32_t)(sl_sleeptimer_tick64_to_ms(ticks));
}
