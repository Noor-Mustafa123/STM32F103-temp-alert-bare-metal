#include <stdint.h>
#include "main.h"
#include "ky_015.h"

extern TIM_HandleTypeDef htim2;

void KY_015_data_reader(void) {
    // TODO: Implement the function to read data from the KY-015 sensor and populate the KY_015_DATA structure.
}

uint8_t delay_execution_in_microseconds_using_TIM2(int microseconds) {
    if (microseconds <= 0 || microseconds > 0xFFFF) {
        return 0; // Invalid input, return 0 to indicate failure
    }

    __HAL_TIM_SET_COUNTER(&htim2, 0);

    HAL_TIM_Base_Start(&htim2);
    
    while (__HAL_TIM_GET_COUNTER(&htim2) < (uint32_t)microseconds) {
        // wait until the TIM2 counter reaches the requested microseconds
    }
    HAL_TIM_Base_Stop(&htim2);

    return 1;
}
