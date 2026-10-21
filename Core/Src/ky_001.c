#include <stdint.h>
#include "main.h"
#include "ky_001.h"

/*
 * KY-001 module = DS18B20 digital temperature sensor + on-board pull-up resistor.
 *
 * The DS18B20 talks over Dallas/Maxim "1-Wire": one data line (PA1) shared
 * by the MCU and the sensor. Neither side ever drives the line HIGH. Each side
 * can only pull it LOW, and the pull-up resistor on the KY-001 board brings it
 * back HIGH when both let go. That is why PA1 is configured as OPEN-DRAIN:
 *
 *   write 0 to the pin  -> MCU pulls the line LOW
 *   write 1 to the pin  -> MCU lets go; the resistor pulls it HIGH
 *                          (unless the sensor is pulling it LOW)
 *
 * On the STM32F1 the input register still reads the real pin level while the
 * pin is an open-drain output, so we never have to switch pin modes
 * (unlike the DHT11 driver).
 *
 * If you ever use a bare DS18B20 instead of the KY-001 module, add a
 * 4.7 kOhm resistor between DATA and 3.3 V.
 *
 * All timing uses TIM2, which ticks once per microsecond (prescaler 7 @ 8 MHz).
 */

/* Fast pin access through the GPIO registers. HAL_GPIO_WritePin() also works,
 * but at 8 MHz its call overhead eats a noticeable part of the 15 us window
 * we have to sample a bit in, so the registers are used directly. */
#define BUS_LOW()      (GPIOA->BRR  = GPIO_PIN_1)        // pull line LOW
#define BUS_RELEASE()  (GPIOA->BSRR = GPIO_PIN_1)        // let go -> HIGH via pull-up
#define BUS_READ()     ((GPIOA->IDR & GPIO_PIN_1) != 0)  // 1 = HIGH, 0 = LOW

/* DS18B20 commands */
#define CMD_SKIP_ROM        0xCC  // talk to the only sensor on the bus
#define CMD_CONVERT_T       0x44  // start a temperature measurement
#define CMD_READ_SCRATCHPAD 0xBE  // read the 9-byte result memory

/* At 12-bit resolution a conversion takes up to 750 ms. */
#define CONVERSION_TIMEOUT_MS 1000

/* The DS18B20 powers up with 85.0 C (raw 0x0550) in its result memory.
 * Reading that value usually means the conversion never happened. */
#define POWER_ON_RESET_RAW 0x0550

/* Busy-wait for a number of microseconds using TIM2. */
static void delay_us(uint32_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    while (__HAL_TIM_GET_COUNTER(&htim2) < us)
    {
    }
}

/*
 * Reset pulse + presence check. Every 1-Wire transaction starts with this.
 *
 *   1. MCU pulls the line LOW for 480 us  ->  "reset"
 *   2. MCU lets go and waits ~70 us
 *   3. If a sensor is connected, it pulls the line LOW  ->  "presence pulse"
 *   4. Wait out the rest of the 480 us presence window
 *
 * Interrupts are briefly disabled so a SysTick interrupt can't stretch the
 * timing. They are re-enabled straight after, so no tick is lost.
 *
 * Returns 0 if a sensor answered, -1 if not.
 */
static int onewire_reset(void)
{
    // the line must be HIGH (idle) before we start; if it is stuck LOW
    // (short circuit, missing pull-up), a "presence" reading would be fake
    if (!BUS_READ())
        return -1;

    __disable_irq();
    BUS_LOW();
    delay_us(480);
    BUS_RELEASE();
    delay_us(70);
    int sensor_present = !BUS_READ();   // sensor pulls LOW if it is there
    __enable_irq();

    delay_us(410);                      // finish the presence window
    return sensor_present ? 0 : -1;
}

/*
 * Send one bit. Every bit is a ~70 us "time slot" that starts with the MCU
 * pulling the line LOW:
 *
 *   bit 1: LOW for ~6 us,  then release for the rest of the slot
 *   bit 0: LOW for ~60 us, then release briefly before the next slot
 *
 * The sensor samples the line ~15-30 us into the slot.
 */
static void onewire_write_bit(int bit)
{
    __disable_irq();
    BUS_LOW();
    if (bit)
    {
        delay_us(6);
        BUS_RELEASE();
        __enable_irq();
        delay_us(64);
    }
    else
    {
        delay_us(60);
        BUS_RELEASE();
        __enable_irq();
        delay_us(10);
    }
}

/*
 * Read one bit. The MCU starts the slot by pulling LOW briefly, then lets go:
 *
 *   sensor sends 1: it does nothing, the line goes back HIGH
 *   sensor sends 0: it keeps the line LOW
 *
 * The MCU must sample within 15 us of the slot start.
 */
static int onewire_read_bit(void)
{
    __disable_irq();
    BUS_LOW();
    delay_us(2);
    BUS_RELEASE();
    delay_us(8);
    int bit = BUS_READ();
    __enable_irq();

    delay_us(55);                       // let the slot finish
    return bit;
}

/* Bytes are sent and received least-significant bit first. */
static void onewire_write_byte(uint8_t byte)
{
    for (int i = 0; i < 8; i++)
    {
        onewire_write_bit(byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t onewire_read_byte(void)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++)
    {
        if (onewire_read_bit())
            byte |= (1u << i);
    }
    return byte;
}

/*
 * Dallas/Maxim CRC-8 (polynomial x^8 + x^5 + x^4 + 1).
 * The sensor's 9th byte is the CRC of the first 8; if ours matches,
 * the data arrived intact. This plays the role of the DHT11 checksum.
 */
static uint8_t crc8_dallas(const uint8_t *data, int length)
{
    uint8_t crc = 0;
    for (int i = 0; i < length; i++)
    {
        uint8_t byte = data[i];
        for (int bit = 0; bit < 8; bit++)
        {
            uint8_t mix = (crc ^ byte) & 0x01;
            crc >>= 1;
            if (mix)
                crc ^= 0x8C;
            byte >>= 1;
        }
    }
    return crc;
}

void KY_001_init(void)
{
    GPIO_InitTypeDef gpio_config = {0};
    gpio_config.Pin   = GPIO_PIN_1;
    gpio_config.Mode  = GPIO_MODE_OUTPUT_OD;   // open-drain, see top of file
    gpio_config.Pull  = GPIO_NOPULL;           // the KY-001 board has the pull-up
    gpio_config.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio_config);

    BUS_RELEASE();                             // idle state: line HIGH
}

/*
 * One full measurement:
 *
 *   1. Reset + SKIP ROM + CONVERT T   -> sensor starts measuring
 *   2. Poll until the sensor reports "done" (read slots return 1)
 *   3. Reset + SKIP ROM + READ SCRATCHPAD -> read 9 bytes
 *   4. Check CRC, then convert bytes 0-1 to degrees
 *
 * Scratchpad layout (9 bytes):
 *   byte 0  temperature LSB
 *   byte 1  temperature MSB      (together: signed, 1/16 C per step)
 *   byte 2  alarm high register
 *   byte 3  alarm low register
 *   byte 4  configuration        (resolution; low 5 bits are always 1)
 *   byte 5-7 reserved
 *   byte 8  CRC of bytes 0-7
 */
int KY_001_data_reader(ds18b20_data_t *data)
{
    /* STEP 1: start a conversion */
    if (onewire_reset() != 0) return -1;
    onewire_write_byte(CMD_SKIP_ROM);
    onewire_write_byte(CMD_CONVERT_T);

    /* STEP 2: wait for it to finish. While measuring, the sensor answers
     * read slots with 0; when done, it answers with 1. */
    uint32_t start_ms = HAL_GetTick();
    while (!onewire_read_bit())
    {
        if (HAL_GetTick() - start_ms > CONVERSION_TIMEOUT_MS)
            return -1;
    }

    /* STEP 3: read the result */
    if (onewire_reset() != 0) return -1;
    onewire_write_byte(CMD_SKIP_ROM);
    onewire_write_byte(CMD_READ_SCRATCHPAD);

    uint8_t bytes[9];
    for (int i = 0; i < 9; i++)
        bytes[i] = onewire_read_byte();

    /* STEP 4: verify and convert */
    if (crc8_dallas(bytes, 8) != bytes[8]) return -1;

    // all-zero data passes the CRC (CRC of zeros is zero), so also check
    // the configuration byte, whose low 5 bits are always 1 on a real sensor
    if ((bytes[4] & 0x1F) != 0x1F) return -1;

    int16_t raw = (int16_t)((bytes[1] << 8) | bytes[0]);
    if (raw == POWER_ON_RESET_RAW) return -1;   // conversion didn't happen

    data->raw         = raw;
    data->temperature = raw / 16.0f;            // 1/16 C per step

    return 0;
}
