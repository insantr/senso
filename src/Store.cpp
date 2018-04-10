//
// Created by Aleksandr Komarica on 30.03.2018.
//

#include <Preferences.h>
#include <vfs_api.h>
#include "Store.h"


bool Store::save(std::vector<sens_data> data) {
    this->pref.begin(_namespace, false);
    this->pref.putUInt("array-size", data.size());
    this->pref.putBytes("array-data", data.data(), (data.size() * sizeof(sens_data)));
    this->pref.end();
    return false;
}

std::vector<sens_data> Store::load() {
    this->pref.begin(_namespace, true);
    uint32_t array_size = this->pref.getUInt("array-size", 0);
    sens_data data[array_size];
    this->pref.getBytes("array-data", data, sizeof(data));
    return std::vector <sens_data> (data, data + sizeof(data) / sizeof(data[0]));
}

bool Store::clear() {
    ESP_EARLY_LOGI(TAG_STORE, "clearData");
    this->pref.begin(_namespace, false);
    bool result = this->pref.clear();
    ESP_EARLY_LOGI(TAG_STORE, "Clear data storeg %d", result);
    this->pref.end();
    return result;
    return false;
}


bool SPIStore::save(std::vector<sens_data> data) {
    File file = this->open("");

    return false;
}

std::vector<sens_data> SPIStore::load() {
    return std::vector<sens_data>();
}

bool SPIStore::clear() {
    return false;
}
