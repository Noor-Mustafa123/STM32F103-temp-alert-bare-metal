#include <stdint.h>
#include "main.h"
#include "ky_015.h"

/*
 * Wait until PA1 reaches the target pin state (HIGH or LOW).
 *
 * Before waiting, the TIM2 counter is reset to zero.
 * The loop checks the pin state on every iteration, and also
 * checks the counter so we do not wait forever if the sensor
 * does not respond.
 *
 * Returns  0 if the pin reached the target state in time.
 * Returns -1 if timeout_us microseconds passed without success.
 */
static int wait_for_pin_state(GPIO_PinState target_state, uint32_t timeout_us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);

    while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) != target_state)
    {
        if (__HAL_TIM_GET_COUNTER(&htim2) >= timeout_us)
            return -1;
    }

    return 0;
}

/*
 * Read one full measurement from the DHT11 sensor on PA1.
 *
 * The DHT11 protocol works like this:
 *
 *   1. MCU pulls the data line LOW for at least 18ms  →  wakes up sensor
 *   2. MCU releases the line HIGH and switches to input mode
 *   3. Sensor responds: pulls LOW ~80us, then HIGH ~80us
 *   4. Sensor sends 40 bits of data, one bit at a time:
 *        - each bit starts with ~50us LOW (preamble, same for every bit)
 *        - then the line goes HIGH for a duration that encodes the bit value:
 *            ~26us HIGH = bit 0
 *            ~70us HIGH = bit 1
 *   5. The 40 bits form 5 bytes in order:
 *        byte 0  humidity integer    (e.g. 55  means 55%)
 *        byte 1  humidity decimal    (always 0 on DHT11)
 *        byte 2  temperature integer (e.g. 24  means 24°C)
 *        byte 3  temperature decimal (always 0 on DHT11)
 *        byte 4  checksum            (must equal bytes 0+1+2+3)
 *
 * Returns  0 and fills *data on success.
 * Returns -1 if the sensor does not respond or checksum fails.
 */
int KY_015_data_reader(DHT11_Data *data)
{
    GPIO_InitTypeDef gpio_config = {0};
    gpio_config.Pin   = GPIO_PIN_1;
    gpio_config.Speed = GPIO_SPEED_FREQ_LOW;

    /* ------------------------------------------------------------------
     * STEP 1: Send the start signal
     *
     * Configure PA1 as push-pull output, pull it LOW for 20ms,
     * then release it HIGH. This wakes the DHT11 up and tells it
     * to start sending a measurement.
     * ------------------------------------------------------------------ */
    gpio_config.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_config.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio_config);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);  // pull LOW  (start signal)
    HAL_Delay(20);                                          // hold for 20ms
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);    // release HIGH

    /* ------------------------------------------------------------------
     * STEP 2: Switch PA1 to input mode
     *
     * The internal pull-up keeps the line HIGH while idle.
     * From here on the sensor drives the line.
     * ------------------------------------------------------------------ */
    gpio_config.Mode = GPIO_MODE_INPUT;
    gpio_config.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &gpio_config);

    /* ------------------------------------------------------------------
     * STEP 3: Wait for the sensor handshake
     *
     * The DHT11 acknowledges the start signal by:
     *   a) pulling the line LOW for ~80us
     *   b) releasing it HIGH for ~80us
     *   c) then pulling LOW again to begin the first data bit
     * ------------------------------------------------------------------ */
    if (wait_for_pin_state(GPIO_PIN_RESET, 100) == -1) return -1;  // (a) sensor pulls LOW
    if (wait_for_pin_state(GPIO_PIN_SET,   200) == -1) return -1;  // (b) sensor releases HIGH
    if (wait_for_pin_state(GPIO_PIN_RESET, 200) == -1) return -1;  // (c) first bit preamble starts

    /* ------------------------------------------------------------------
     * STEP 4: Read 40 bits
     *
     * For each bit:
     *   - Wait for the LOW preamble to finish (pin goes HIGH)
     *   - Measure how long the pin stays HIGH using the TIM2 counter
     *   - HIGH < 40us → bit 0,  HIGH > 40us → bit 1
     *   - Pack the bit into the correct position in the bytes array
     *
     * Bits arrive MSB first. Bit 0 is the MSB of byte 0.
     * ------------------------------------------------------------------ */
    uint8_t bytes[5] = {0};

    for (int bit_index = 0; bit_index < 40; bit_index++)
    {
        // wait for LOW preamble to end — the rising edge marks the start of the HIGH window
        if (wait_for_pin_state(GPIO_PIN_SET, 100) == -1) return -1;

        // measure how long the pin stays HIGH
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        if (wait_for_pin_state(GPIO_PIN_RESET, 150) == -1) return -1;
        uint32_t high_duration_us = __HAL_TIM_GET_COUNTER(&htim2);

        // decide the bit value based on HIGH duration
        int bit_value = (high_duration_us > 40) ? 1 : 0;

        // pack bit into the correct byte (bits arrive MSB first)
        int byte_index   = bit_index / 8;
        int bit_position = 7 - (bit_index % 8);
        bytes[byte_index] |= (bit_value << bit_position);
    }

    /* ------------------------------------------------------------------
     * STEP 5: Verify checksum
     *
     * The sensor appends a checksum byte (bytes[4]) that must equal
     * the sum of the first four bytes. If it does not match, the
     * data was corrupted and we discard it.
     * ------------------------------------------------------------------ */
    uint8_t expected_checksum = bytes[0] + bytes[1] + bytes[2] + bytes[3];
    if (expected_checksum != bytes[4]) return -1;

    /* ------------------------------------------------------------------
     * STEP 6: Store the result
     * ------------------------------------------------------------------ */
    data->humidity    = bytes[0];  // byte 0: humidity integer part
    data->temperature = bytes[2];  // byte 2: temperature integer part

    return 0;
}
