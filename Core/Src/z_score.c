#include "z_score.h"
#include "main.h"
#include <math.h>

int calculate_z_score_preliminary_data(int sample_size, z_score_preliminary_data_t *preliminary_data)
{
    uint8_t readings[sample_size];

    // collect all samples once, with a delay between each read
    for (int i = 0; i < sample_size; i++)
    {
        dht11_data_t data;
        if (KY_015_data_reader(&data) != 0)
        {
            return -1;
        }
        readings[i] = data.temperature;
        if (i < sample_size - 1)
        {
            HAL_Delay(2000);
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

    preliminary_data->average_temperature = average;
    preliminary_data->standard_deviation = std_dev;

    return 0;
}

float calculate_z_score(z_score_preliminary_data_t preliminary_data, dht11_data_t dht11_data)
{
    if (preliminary_data.standard_deviation == 0.0f)
    {
        return 0.0f;
    }
    float difference = dht11_data.temperature - preliminary_data.average_temperature;
    return difference / preliminary_data.standard_deviation;
}
