#ifndef __KY_001_H
#define __KY_001_H

#include <stdint.h>
#include "main.h"

/*
 * Holds one measurement from the DS18B20 sensor on the KY-001 module.
 *
 * The DS18B20 reports temperature in steps of 1/16 °C (0.0625 °C) and
 * measures from -55 °C to +125 °C. There is no humidity reading.
 */
typedef struct {
    int16_t raw;          // raw sensor value, in 1/16 °C steps (e.g. 616 = 38.5 °C)
    float   temperature;  // degrees Celsius (raw / 16)
} ds18b20_data_t;

/*
 * Configure PA1 for the 1-Wire bus. Call once at start-up,
 * after MX_GPIO_Init() and HAL_TIM_Base_Start(&htim2).
 */
void KY_001_init(void);

/*
 * Start a temperature conversion, wait for it to finish (up to ~750 ms),
 * then read the result from the DS18B20 on PA1.
 *
 * Returns  0 and fills *data on success.
 * Returns -1 if no sensor answered, the conversion timed out,
 *            or the CRC check failed.
 */
int KY_001_data_reader(ds18b20_data_t *data);

#endif /* __KY_001_H */
