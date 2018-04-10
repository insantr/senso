//
// Created by Aleksandr Komarica on 01.04.2018.
//

//#include <WString.h>
//#include <cstdint>
//#include <Esp.h>

//#include "../../../.platformio/packages/framework-arduinoespressif32/cores/esp32/WString.h"

String getChipId();

struct sens_data{
    float temp;
    float hum;
    float pres;
    uint32_t volt;
    float chip_temp;
    time_t time; // time in sec
};