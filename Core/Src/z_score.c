int calculate_average_temperature(int num_readings) {
    int total_temperature = 0;
    for (int i = 0; i < num_readings; i++) {
        DHT11_Data data;
        if (KY_015_data_reader(&data) != 0) {
            // todo: Handle error if sensor reading fails
            return -1;
        }
        total_temperature += data.temperature;
    }
    return total_temperature / num_readings;
}

int calculate_z_score() {
    // calculate initial average temperature
    int average_temperature = calculate_average_temperature(5);

    // get initial data from ky_015 sensor
    DHT11_Data data;
    if (KY_015_data_reader(&data) != 0) {
        // todo: Handle error if sensor reading fails
        return -1;
    }


    
}

