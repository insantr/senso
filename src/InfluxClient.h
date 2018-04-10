//
// Created by Aleksandr Komarica on 01.04.2018.
//

#ifndef ESP32_SENSORS_INFUXCLIENT_H
#define ESP32_SENSORS_INFUXCLIENT_H


#include <HTTPClient.h>
#include <vector>
#include "utils.h"

static const char* TAG_INFLUX_CLIENT = "InfluxClient";

class InfluxClient: HTTPClient {
private:
    String url_to_db;
public:
    InfluxClient(String url_to_db):url_to_db(url_to_db){};

    bool send(std::vector<sens_data> data, uint32_t repeat);
    bool send(std::vector<sens_data> data);

};


#endif //ESP32_SENSORS_INFUXCLIENT_H
