#include <stdint.h>
#include "main.h"
#include "ky_015.h"

#define TRANSMISSION_SIGNAL_DELAY 50


uint8_t delay_execution_in_microseconds_using_TIM2(int microseconds)
{
    if (microseconds <= 0) return 1;
    if (htim2.Instance == NULL) return 1;
    uint32_t arr = htim2.Instance->ARR;
    if ((uint32_t)microseconds > arr) return 1;
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while ((__HAL_TIM_GET_COUNTER(&htim2)) < (uint32_t)microseconds);
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    return 0;
}

static void pc13_blink(int times)
{
    for (int i = 0; i < times; i++) {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        HAL_Delay(100);
    }
}

/* Wait for next edge captured by TIM2 CH2, return captured value or -1 on timeout */
static int wait_for_edge(uint32_t timeout_count)
{
    __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_CC2);
    while (!__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_CC2) && timeout_count--);
    if (timeout_count == 0) return -1;
    return (int)HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_2);
}

int KY_015_data_reader()
{
    // STEP 1: send start signal - pull PA1 LOW for 18ms then release HIGH
    GPIO_InitTypeDef GPIO_init = {0};
    GPIO_init.Pin   = GPIO_PIN_1;
    GPIO_init.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_init.Pull  = GPIO_NOPULL;
    GPIO_init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_init);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_Delay(18);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
    HAL_Delay(1);  // hold HIGH 20-40us (1ms is safe margin)

    // STEP 2: switch PA1 to input, start TIM2 Input Capture
    GPIO_init.Mode = GPIO_MODE_INPUT;
    GPIO_init.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_init);

    HAL_TIM_Base_Start(&htim2);
    HAL_TIM_IC_Start(&htim2, TIM_CHANNEL_2);

    // STEP 3: handshake - DHT11 pulls LOW 80us (falling edge)
    if (wait_for_edge(200000) == -1) {
        HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_2);
        return -1;
    }
    pc13_blink(1);  // 1 blink = DHT11 pulled LOW (alive)

    // STEP 4: handshake - DHT11 releases HIGH 80us (rising edge)
    if (wait_for_edge(200000) == -1) {
        HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_2);
        return -1;
    }
    pc13_blink(2);  // 2 blinks = handshake complete, ready to receive data

    // STEP 5: read 40 data bits using Input Capture
    // each bit: falling edge (50us LOW) -> rising edge -> falling edge (26us=0, 70us=1)
    uint8_t bytes[5] = {0, 0, 0, 0, 0};

    for (int i = 0; i < 40; i++) {
        int t1, t2;

        // wait for falling edge = start of 50us LOW preamble
        if (wait_for_edge(20000) == -1) { HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_2); return -1; }

        // wait for rising edge = end of LOW preamble, start of data HIGH
        t1 = wait_for_edge(20000);
        if (t1 == -1) { HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_2); return -1; }

        // wait for falling edge = end of data HIGH
        t2 = wait_for_edge(20000);
        if (t2 == -1) { HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_2); return -1; }

        uint32_t duration = (uint32_t)(t2 - t1);
        int bit = (duration > 40) ? 1 : 0;
        bytes[i / 8] |= (bit << (7 - (i % 8)));
    }

    HAL_TIM_IC_Stop(&htim2, TIM_CHANNEL_2);

    // TODO: parse bytes - humidity=bytes[0], temp=bytes[2], checksum=bytes[4]
    return 0;
}
