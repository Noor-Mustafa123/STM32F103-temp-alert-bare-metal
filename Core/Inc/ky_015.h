#ifndef __KY_015_H
#define __KY_015_H

#include <stdint.h>
#include "main.h"

/*
 * Holds one measurement from the DHT11 sensor.
 * Both fields are integers — the DHT11 does not report decimal places.
 */
typedef struct {
    uint8_t temperature;  // degrees Celsius
    uint8_t humidity;     // percent relative humidity
} DHT11_Data;

/*
 * Read temperature and humidity from the DHT11 on PA1.
 * Returns 0 on success, -1 if the sensor did not respond or checksum failed.
 */
int KY_015_data_reader(DHT11_Data *data);

#endif /* __KY_015_H */
