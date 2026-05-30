#include <stdint.h>
#include "main.h"
#include "ky_015.h"


void KY_015_data_reader(void) {
    // TODO: Implement the function to read data from the KY-015 sensor and populate the KY_015_DATA structure.
}

uint8_t delay_execution_in_microseconds_using_TIM2(int microseconds) {
    if (microseconds <= 0) {
        return 1; // invalid input
    }

    if (htim2.Instance == NULL) {
        return 1; // TIM2 not configured
    }

    uint32_t arr = htim2.Instance->ARR;
    if ((uint32_t)microseconds > arr) {
        return 1; // requested delay exceeds timer auto-reload
    }

    /* Reset counter then busy-wait until requested ticks (assumes 1 tick = 1 us) */
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while ((__HAL_TIM_GET_COUNTER(&htim2)) < (uint32_t)microseconds) {
        /* busy wait */
    }
    __HAL_TIM_SET_COUNTER(&htim2, 0);

    return 0; // success
}

void KY015_SetDataPinMode(GPIO_TypeDef *GPIOx,
                          uint16_t pin,
                          uint32_t mode,
                          uint32_t pull)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = pin;
    gpio.Mode = mode;
    gpio.Pull = pull;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOx, &gpio);
}