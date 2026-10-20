#ifndef __KY_015_H
#define __KY_015_H

#include <stdint.h>
#include "main.h"

/*
 * Holds one measurement from the DHT11 sensor.
 * Both fields are integers — the DHT11 does not report decimal places.
 */
typedef struct {
    // todo: get more data from sensor output and incldue in struct
    uint8_t temperature;  // degrees Celsius
    uint8_t humidity;     // percent relative humidity
} dht11_data_t;

/*
 * Read temperature and humidity from the DHT11 on PA1.
 * Returns 0 on success, -1 if the sensor did not respond or checksum failed.
 */
int KY_015_data_reader(dht11_data_t *data);

#endif /* __KY_015_H */
