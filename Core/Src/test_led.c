#include "main.h"
#include "stm32f1xx_hal.h"

/**
 * @brief Initialize test LED on GPIO pin
 * Configure GPIOB PIN_0 as output for testing
 */
void test_led_init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // Enable GPIOB clock if not already enabled
    __HAL_RCC_GPIOB_CLK_ENABLE();
    
    // Configure PB0 as output
    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);  // Start LED off
}

/**
 * @brief Blink LED N times as confirmation
 * Use this to confirm code sections are executing
 * @param blink_count: Number of times to blink (1-5 recommended)
 * @param delay_ms: Milliseconds per blink on/off
 */
void test_led_blink(uint8_t blink_count, uint16_t delay_ms)
{
    for (uint8_t i = 0; i < blink_count; i++)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);   // LED ON
        HAL_Delay(delay_ms);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); // LED OFF
        HAL_Delay(delay_ms);
    }
}

/**
 * @brief Pulse LED to indicate success
 * Quick double-blink pattern
 */
void test_led_success(void)
{
    test_led_blink(2, 100);  // 2 quick blinks = SUCCESS
}

/**
 * @brief Pulse LED to indicate error
 * Slower triple-blink pattern
 */
void test_led_error(void)
{
    test_led_blink(3, 200);  // 3 slower blinks = ERROR
}
