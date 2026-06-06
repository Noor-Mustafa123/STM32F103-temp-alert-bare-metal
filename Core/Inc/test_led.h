#ifndef TEST_LED_H
#define TEST_LED_H

#include <stdint.h>

/**
 * @brief Initialize test LED
 */
void test_led_init(void);

/**
 * @brief Blink LED N times
 * @param blink_count: Number of blinks
 * @param delay_ms: Delay per blink
 */
void test_led_blink(uint8_t blink_count, uint16_t delay_ms);

/**
 * @brief Success pattern (2 quick blinks)
 */
void test_led_success(void);

/**
 * @brief Error pattern (3 slower blinks)
 */
void test_led_error(void);

#endif /* TEST_LED_H */
