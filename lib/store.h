//
// Created by Aleksandr Komarica on 30.03.2018.
//

#ifndef ESP32_SENSORS_STORE_H
#define ESP32_SENSORS_STORE_H

struct sens_data{
    float temp;
    float hum;
    float pres;
    uint32_t volt;
    float chip_temp;
    time_t time; // time in sec
};

template <typename _data>
class Store{
public:
    bool save(std::vector <_data> data);
    std::vector <_data> load();
    bool clear();
};

#endif //ESP32_SENSORS_STORE_H
