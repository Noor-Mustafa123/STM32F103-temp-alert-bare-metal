#include <z_score.h>
#include <ky_015.h>
#include <math.h>

int calculate_average_temperature(int num_readings)
{
    int total_temperature = 0;
    for (int i = 0; i < num_readings; i++)
    {
        dht11_data_t data;
        if (KY_015_data_reader(&data) != 0)
        {
            return -1;
        }
        total_temperature += data.temperature;
    }
    return total_temperature / num_readings;
}

int calculate_standard_deviation(int num_readings, int average_temperature)
{
    int squared_deviations[num_readings];
    for (int i = 0; i < num_readings; i++)
    {
        dht11_data_t data;
        if (KY_015_data_reader(&data) != 0)
        {
            return -1;
        }
        int deviation = average_temperature - data.temperature;
        squared_deviations[i] = deviation * deviation;
    }

    int sum_of_deviations;
    for (int i = 0; i < sizeof(squared_deviations); i++)
    {
        sum_of_deviations += squared_deviations[i];
    }
    int mean = sum_of_deviations / sizeof(squared_deviations);
    int standard_deviation = sqrt(mean);
    return standard_deviation;
}

int calculate_z_score_preliminary_data(int sample_size, z_score_preliminary_data_t *preliminary_data)
{

    // calculate initial average temperature
    int average_temperature = calculate_average_temperature(sample_size);
    if (average_temperature == -1)
    {
        return -1;
    }

    // calculate standard deviation
    int standard_deviation = calculate_standard_deviation(sample_size, average_temperature);
    if (standard_deviation == -1)
    {
        return -1;
    }

    preliminary_data->average_temperature = average_temperature;
    preliminary_data->standard_deviation = standard_deviation;

    return 0;
}

int calculate_z_score(z_score_preliminary_data_t preliminary_data, dht11_data_t dht11_data)
{
        int difference = dht11_data.temperature - preliminary_data.average_temperature;
        int z_score = difference / preliminary_data.standard_deviation;
        return z_score;
}
