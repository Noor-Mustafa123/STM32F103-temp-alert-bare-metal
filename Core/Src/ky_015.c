#include <stdint.h>
#include "main.h"
#include "ky_015.h"

#define TRANSMISSION_SIGNAL_DELAY 50


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

int transmission_signal_parser(){
    while( HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET ); // wait until pull is up
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while( HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET ); // wait until pull is down
    int duration= __HAL_TIM_GET_COUNTER(&htim2);
    
    int bit;
    
    if(duration <= 40)   // use 40 as threshold between 26us and 70us
        bit = 0;
    else
        bit = 1;
    return bit;
}


void KY_015_data_reader()
{
    // send out start signals to DHT11 sensor 
    set_ky_015_data_pin_mode( GPIO_MODE_OUTPUT_PP, GPIO_NOPULL ); 
    
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET); 
    HAL_Delay(18);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
    delay_execution_in_microseconds_using_TIM2(20);

    // sensor confirmation protocol signals prior to data transmission
    set_ky_015_data_pin_mode(GPIO_MODE_INPUT, GPIO_PULLUP);
    while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_SET);
    while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET); 
    delay_execution_in_microseconds_using_TIM2(80);

    // transmission start
    
    int resolved_bit[40]; 

    for( int i = 0; i < 40; i++ ){
     resolved_bit[i] = transmission_signal_parser();
    }

    
}