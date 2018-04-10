//
// Created by Aleksandr Komarica on 30.03.2018.
//

#ifndef ESP32_SENSORS_STORE_H
#define ESP32_SENSORS_STORE_H


#include <vector>
#include <sys/types.h>
#include <Preferences.h>
#include <SPIFFS.h>
#include "utils.h"

#define DEFAULT_STORE_NAMESPACE "sensors"

#ifdef LOG_LOCAL_LEVEL
#undef LOG_LOCAL_LEVEL
#endif
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE

static const char* TAG_STORE = "Store";

class Store{
public:
    Store():_namespace(DEFAULT_STORE_NAMESPACE){};
    Store(const char* _namespace):_namespace(_namespace){};

    bool save(std::vector <sens_data> data);
    std::vector <sens_data> load();
    bool clear();

private:
    const char* _namespace;
    Preferences pref;
};

class SPIStore : public fs::SPIFFSFS {
public:
    SPIStore::SPIStore(const fs::FSImplPtr &impl) : SPIFFSFS(impl) {}
    bool save(std::vector <sens_data> data);
    std::vector <sens_data> load();
    bool clear();
};

SPIStore StoreData = SPIStore(FSImplPtr(new VFSImpl()));
//SPIFFS = 1;


#endif //ESP32_SENSORS_STORE_H
