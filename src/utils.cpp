//
// Created by Aleksandr Komarica on 01.04.2018.
//

#include <WString.h>
#include <cstdint>
#include <Esp.h>
#include "utils.h"

String getChipId(){
    uint64_t chipid = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
    String str_chip_id = String((uint16_t)(chipid>>32), HEX);
    str_chip_id.concat(String((uint32_t)chipid, HEX));
    return str_chip_id;
}