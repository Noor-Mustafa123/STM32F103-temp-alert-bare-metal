#include <stdint.h>
#include "main.h"
#include "ky_015.h"

void KY_015_data_reader()
{
   
    set_ky_015_data_pin_mode( GPIO_MODE_OUTPUT_PP, GPIO_NOPULL ); 
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET); 
    HAL_Delay(18);

}

uint8_t delay_execution_in_microseconds_using_TIM2(int microseconds)
{
    if (microseconds <= 0)
    {
        return 1; // invalid input
    }

    if (htim2.Instance == NULL)
    {
        return 1; // TIM2 not configured
    }

    uint32_t arr = htim2.Instance->ARR;
    if ((uint32_t)microseconds > arr)
    {
        return 1; // requested delay exceeds timer auto-reload
    }

    /* Reset counter then busy-wait until requested ticks (assumes 1 tick = 1 us) */
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while ((__HAL_TIM_GET_COUNTER(&htim2)) < (uint32_t)microseconds)
    {
        /* busy wait */
    }
    __HAL_TIM_SET_COUNTER(&htim2, 0);

    return 0; // success
}

void set_ky_015_data_pin_mode(uint32_t mode, uint32_t default_pull)
{
    GPIO_InitTypeDef GPIO_init_struct = {0};
    GPIO_init_struct.Pin = GPIO_PIN_3;
    GPIO_init_struct.Mode = mode;
    GPIO_init_struct.Pull = default_pull;
    GPIO_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_init_struct);
}