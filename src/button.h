/**
 * @file button.h
 * @brief Button driver for PB13 with debounce and press detection
 *
 * Implements short press and long press detection with proper debouncing.
 * - Short press: < 1 second
 * - Long press: >= 3 seconds
 */

#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>
#include <stdbool.h>

//==============================================================================
// Configuration
//==============================================================================

// Button pin (PB13 = Pin 19)
#define BUTTON_PORT         gpioPortB
#define BUTTON_PIN          13

// Timing configuration
#define BUTTON_DEBOUNCE_MS  50      // Debounce time
#define BUTTON_LONG_PRESS_MS 3000   // Long press threshold

//==============================================================================
// Public Functions
//==============================================================================

/**
 * @brief Initialize button driver
 * Configures GPIO with pull-up and interrupt
 */
void button_init(void);

/**
 * @brief Process button state machine
 * Must be called periodically from main loop
 */
void button_process(void);

/**
 * @brief Check if button is currently pressed
 * @return true if button is pressed (active low)
 */
bool button_is_pressed(void);

//==============================================================================
// Callback Functions (to be implemented by application)
//==============================================================================

/**
 * @brief Short press callback
 * Called when button is pressed and released within 1 second
 */
void button_short_press_callback(void);

/**
 * @brief Long press callback
 * Called when button is held for 3+ seconds
 */
void button_long_press_callback(void);

#endif // BUTTON_H
