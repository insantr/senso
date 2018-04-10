//
// Created by Aleksandr Komarica on 01.04.2018.
//

#include "InfluxClient.h"

bool InfluxClient::send(std::vector<sens_data> data, uint32_t repeat) {
    String payload;
    String chip_id = getChipId();

    for(uint16_t i=0; i < data.size(); i++){
        if(i > 0){
            payload += String("\n");
        }
        payload += String("temperature,sensorId=") + String(chip_id) + String(" value=") + String(data[i].temp, 6)
                  + String(" ") + String(data[i].time) + String("000000000");
        payload += String("\nhumidity,sensorId=") + String(chip_id) + String(" value=") + String(data[i].hum, 6)
                  + String(" ") + String(data[i].time) + String("000000000");
        payload += String("\npressure,sensorId=") + String(chip_id) + String(",unit=pascal value=")
                  + String(data[i].pres, 6) + String(" ") + String(data[i].time) + String("000000000");
        payload += String("\npressure,sensorId=") + String(chip_id) + String(",unit=mmHg value=")
                  + String(data[i].pres / 133.322387415, 6) + String(" ") + String(data[i].time) + String("000000000");
        payload += String("\nbattery,sensorId=") + String(chip_id) + String(" value=") + String(data[i].volt)
                  + String(" ") + String(data[i].time) + String("000000000");
        payload += String("\nchip_temp,sensorId=") + String(chip_id) + String(" value=") + String(data[i].chip_temp, 6)
                  + String(" ") + String(data[i].time) + String("000000000");
    }

    this->begin("http://insantr.od.ua:8086/write?db=weather");
    t_http_codes httpCode = (t_http_codes) this->POST(payload);
    for(uint32_t left_try = repeat; (left_try > 0 || repeat == 0); left_try--){
        switch (httpCode){
            case t_http_codes::HTTP_CODE_NO_CONTENT: {
                ESP_EARLY_LOGI(TAG_INFLUX_CLIENT, "[HTTP] POST... code: %d", httpCode);
                return true;
            }
            default: {
                httpCode = (t_http_codes) this->POST(payload);
                ESP_LOGE(TAG, "[HTTP] POST... code: %d; left_try: %d;", httpCode, left_try);
            };
        };
    }
    return (httpCode == HTTP_CODE_NO_CONTENT);
}

bool InfluxClient::send(std::vector<sens_data> data) {
    return this->send(data, 1);
}