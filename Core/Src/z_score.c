#include "z_score.h"
#include "main.h"
#include <math.h>

int calculate_z_score_preliminary_data(int sample_size, z_score_preliminary_data_t *preliminary_data)
{
    uint8_t readings[sample_size];
    int collected = 0;
    int failures = 0;

    // collect samples until we have enough good ones, skipping failed reads
    while (collected < sample_size)
    {
        dht11_data_t data;
        if (KY_015_data_reader(&data) == 0)
        {
            readings[collected] = data.temperature;
            collected++;
        }
        else
        {
            failures++;
            if (failures > Z_SCORE_MAX_READ_FAILURES)
            {
                return -1;
            }
        }

        if (collected < sample_size)
        {
            HAL_Delay(Z_SCORE_LEARN_INTERVAL_MS);
        }
    }

    // calculate average from the collected samples
    float total = 0.0f;
    for (int i = 0; i < sample_size; i++)
    {
        total += readings[i];
    }
    float average = total / sample_size;

    // calculate standard deviation from the same samples
    float sum_sq = 0.0f;
    for (int i = 0; i < sample_size; i++)
    {
        float deviation = readings[i] - average;
        sum_sq += deviation * deviation;
    }
    float variance = sum_sq / sample_size;
    float std_dev = sqrtf(variance);

    // a std dev of 0 (all readings identical) would switch detection off
    if (std_dev < Z_SCORE_MIN_STD_DEV)
    {
        std_dev = Z_SCORE_MIN_STD_DEV;
    }

    preliminary_data->average_temperature = average;
    preliminary_data->standard_deviation = std_dev;

    return 0;
}

float calculate_z_score(z_score_preliminary_data_t preliminary_data, dht11_data_t dht11_data)
{
    float difference = dht11_data.temperature - preliminary_data.average_temperature;
    return difference / preliminary_data.standard_deviation;
}
