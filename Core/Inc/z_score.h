#ifndef Z_SCORE_H
#define Z_SCORE_H

typedef struct
{
    int standard_deviation;
    int average_temperature;
} z_score_preliminary_data_t;

int calculate_z_score_preliminary_data(int sample_size, z_score_preliminary_data_t *preliminary_data);

int calculate_z_score(z_score_preliminary_data_t preliminary_data, dht11_data_t dht11_data);

#endif