#ifndef Z_SCORE_H
#define Z_SCORE_H

#include "ky_001.h"

#define Z_SCORE_SAMPLE_SIZE 30

// wait for the fan to warm up so the warm-up climb isn't learned as normal
#define Z_SCORE_WARMUP_MS 60000

// space learning reads out so they cover the fan's natural variation (30 x 10 s = 5 min)
#define Z_SCORE_LEARN_INTERVAL_MS 10000

// give up on a learning run after this many failed sensor reads
#define Z_SCORE_MAX_READ_FAILURES 10

// floor for the standard deviation, so identical readings can't disable detection.
// The DS18B20 resolves 0.0625 C (the DHT11 only whole degrees, which needed 0.5),
// so a smaller floor keeps detection sensitive.
#define Z_SCORE_MIN_STD_DEV 0.1f

// |z| above this counts as an anomaly
#define Z_SCORE_THRESHOLD 3.0f

typedef struct
{
    float standard_deviation;
    float average_temperature;
} z_score_preliminary_data_t;

int calculate_z_score_preliminary_data(int sample_size, z_score_preliminary_data_t *preliminary_data);

float calculate_z_score(z_score_preliminary_data_t preliminary_data, ds18b20_data_t sensor_data);

#endif
