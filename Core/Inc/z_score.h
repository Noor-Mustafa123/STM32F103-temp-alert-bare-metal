#ifndef Z_SCORE_H
#define Z_SCORE_H

#include "ky_015.h"

#define Z_SCORE_SAMPLE_SIZE 30

typedef struct
{
    float standard_deviation;
    float average_temperature;
} z_score_preliminary_data_t;

int calculate_z_score_preliminary_data(int sample_size, z_score_preliminary_data_t *preliminary_data);

float calculate_z_score(z_score_preliminary_data_t preliminary_data, dht11_data_t dht11_data);

#endif
