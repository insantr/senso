#ifdef LOG_LOCAL_LEVEL
#undef LOG_LOCAL_LEVEL
#endif
#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#undef BOOTLOADER_BUILD
#define CONFIG_LOG_COLORS true

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include "BME280I2C.h"
#include "Store.h"
#include <HTTPClient.h>
#include <esp_smartconfig.h>
#include <esp_wifi.h>
#include <EEPROM.h>
//#include "NTPClient.h"
#define COUNT_OF(array) (sizeof(array) / sizof(array[0]))
#include <vector>
#include <lwipopts.h>
#include <esp_adc_cal.h>
#import "utils.h"
#include "InfluxClient.h"
#include "config.h"



// platformio serialports monitor --port /dev/cu.SLAB_USBtoUART --baud 115200
#define TIMEOUT_INIT_SERIAL 5000
#define DEBUG true
#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  60*30        /* Time ESP32 will go to sleep (in seconds) */
static const char* TAG = "MyModule";

//#define USE_SMART_CONFIG false
//
//#define CONFIG_LOG_COLORS true


//
//#include <esp_log.h>

const char *NTP_SERVER_1 = "0.ua.pool.ntp.org";
const char *NTP_SERVER_2 = "1.ua.pool.ntp.org";
const char *NTP_SERVER_3 = "2.ua.pool.ntp.org";
const char *TZ_INFO = "UTC";
//const char *TZ_INFO = "EST5EDT4,M3.2.0/02:00:00,M11.1.0/02:00:00";


BME280I2C bme;
HTTPClient http;
Store store;
InfluxClient influx("http://insantr.od.ua:8086/write?db=weather");

//void prepareData();

void go_to_sleep(uint32_t sec=TIME_TO_SLEEP) {
    ESP_EARLY_LOGI(TAG, "Setup to sleep for every %dsec and going to sleep.", TIME_TO_SLEEP);
    ESP.deepSleep(sec * uS_TO_S_FACTOR);
}

//wl_status_t start_smart_config() {
//    wl_status_t status = WL_DISCONNECTED;
//    do {
//        Serial.println("Try use Smart Connect");
//        WiFi.mode(WIFI_AP_STA);
//        Serial.println("[SMARTCONFIG]: Version: " + String(esp_smartconfig_get_version()));
//        Serial.println("Begin smart config: " + String(WiFi.beginSmartConfig()));
//        Serial.println("Waiting for SmartConfig.");
//        while (!WiFi.smartConfigDone()) {
//            delay(500);
//            Serial.print(".");
//        }
//        Serial.println("");
//        Serial.println("SmartConfig received.");
//        Serial.println("Waiting for WiFi");
//        status = (wl_status_t) WiFi.waitForConnectResult();
//        if (status == WL_CONNECTED) {
//            Serial.println("WiFi Connected.");
//            Serial.print("IP Address: ");
//            Serial.println(WiFi.localIP());
//            Serial.println("SSID: " + WiFi.SSID() + "; psk: " + WiFi.psk());
//            return status;
//        }
//    } while (status != WL_CONNECTED);
//}

/**
 * Connect to WiFi
 * @param repeat Count of repeat before fail. 0 = Inf
 * @return true if connect to wifi is established
 */
bool connect_to_wifi(int repeat = 0) {
    ESP_EARLY_LOGI(TAG, "Try connect to WiFi");
    if (WiFi.isConnected() == WL_CONNECTED) {
        ESP_EARLY_LOGI(TAG, "WiFi already connected");
        ESP_EARLY_LOGI(TAG, "SSID: '%s', PASS: '%s'", WiFi.SSID().c_str(), WiFi.psk().c_str());
        return true;
    }
    ESP_EARLY_LOGI(TAG, "SSID: '%s' PASS: '%s'", SSID_WIFI, PASS_WIFI);
    wl_status_t status;
    while(WL_CONNECT_FAILED == WiFi.begin(SSID_WIFI, PASS_WIFI)){
        ESP_LOGE(TAG, "Failed start WiFi");
    };
    status = (wl_status_t) WiFi.waitForConnectResult();
    for (int left_try = repeat; left_try > 0 || repeat == 0; left_try--) {
        switch (status) {
            case WL_CONNECTED:
                ESP_EARLY_LOGI(TAG, "Connection to WiFi(%s) is established", WiFi.SSID().c_str());
                ESP_EARLY_LOGD(TAG, "SSID: '%s'; LocalIP: '%s'; macAddress: '%s'; Hostname: '%s';",
                               WiFi.SSID().c_str(), WiFi.localIP().toString().c_str(), WiFi.macAddress().c_str(), WiFi.getHostname());
                return (status == WL_CONNECTED);

            case WL_NO_SHIELD:
            case WL_IDLE_STATUS:
            case WL_NO_SSID_AVAIL:
            case WL_SCAN_COMPLETED:
            case WL_CONNECT_FAILED:
            case WL_CONNECTION_LOST:
            case WL_DISCONNECTED:
                ESP_LOGE(TAG, "Connection to WiFi(%s) is not established; Status: %d;", WiFi.SSID().c_str(), status);
                // TODO: Implement Smart Config when fix android app
                if (WiFi.reconnect()) {
                    WiFi.disconnect(true);
                    while(WL_CONNECT_FAILED == WiFi.begin(SSID_WIFI, PASS_WIFI)){
                        ESP_LOGE(TAG, "Failed start WiFi");
                    };
                    status = (wl_status_t) WiFi.waitForConnectResult();
                    ESP_EARLY_LOGI(TAG, "Reconnect[%d] is OK, new status = %d", abs(left_try), status);
                } else {
                    ESP_LOGE(TAG, "Fail while reconnect");
                }
        }
        delay(status != WL_CONNECTED ? 5000 : 0);
    }
    return (status == WL_CONNECTED);
}

void init_ntp_client_and_update_time() {
    ESP_EARLY_LOGI(TAG, "Try config NTP");
    configTzTime(TZ_INFO, NTP_SERVER_1, NTP_SERVER_2, NTP_SERVER_3);
    if (connect_to_wifi()) {
        struct tm timeinfo;
        while(!getLocalTime(&timeinfo, 10000)){
            ESP_EARLY_LOGE(TAG, "NTP NOT configured. ");
        }
        ESP_EARLY_LOGI(TAG, "NTP configured. ");
        char strftime_buf[64];
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_EARLY_LOGI(TAG, "The current date/time is: %s", strftime_buf);

        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    }
}

void _init(){
//    delay(10000);

    Serial.begin(SERIAL_BAUD);

    esp_log_level_set("*", ESP_LOG_ERROR);
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_STORE, ESP_LOG_VERBOSE);
    esp_log_level_set(TAG_INFLUX_CLIENT, ESP_LOG_VERBOSE);

    Wire.begin();

    StoreData.begin(true);

    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);

    delay(3000);
}

//struct sens_data{
//    float temp;
//    float hum;
//    float pres;
//    uint32_t volt;
//    float chip_temp;
//    time_t time; // time in sec
//};



//void printSensData(sens_data data, String pref="", int num=0){
//    Serial.printf(">>> %30s [%d] temperature: %.6f; humidity: %.6f; pressure: %.6f; battery: %.6f; chip_temp: %.6f; time: %d;\n",
//            pref.c_str(), num, data.temp, data.hum, data.pres, data.volt, data.chip_temp, data.time);
//}
//void printSensData(std::vector <sens_data> data, String pref=""){
//    Serial.printf("[Vector info] Size: %d\n", data.size());
//    for(int i=0; i<data.size(); i++){
//        printSensData(data[i], pref, i);
//    }
//}
/*
String prepare_data_for_influx(std::vector <sens_data> data){
    String chip_id = getChipId();
    String result = "";
    for(uint16_t i=0; i < data.size(); i++){
        if(i > 0){
            result += String("\n");
        }
        result += String("temperature,sensorId=") + String(chip_id) + String(" value=") + String(data[i].temp, 6)
                  + String(" ") + String(data[i].time) + String("000000000");
        result += String("\nhumidity,sensorId=") + String(chip_id) + String(" value=") + String(data[i].hum, 6)
                  + String(" ") + String(data[i].time) + String("000000000");
        result += String("\npressure,sensorId=") + String(chip_id) + String(",unit=pascal value=")
                  + String(data[i].pres, 6) + String(" ") + String(data[i].time) + String("000000000");
        result += String("\npressure,sensorId=") + String(chip_id) + String(",unit=mmHg value=")
                  + String(data[i].pres / 133.322387415, 6) + String(" ") + String(data[i].time) + String("000000000");
        result += String("\nbattery,sensorId=") + String(chip_id) + String(" value=") + String(data[i].volt)
                  + String(" ") + String(data[i].time) + String("000000000");
        result += String("\nchip_temp,sensorId=") + String(chip_id) + String(" value=") + String(data[i].chip_temp, 6)
                  + String(" ") + String(data[i].time) + String("000000000");
    }
    return result;
}
*/
/**
 *
 * @param data
 * @param repeat
 * @return
 */
/*
bool send_data_to_influx(String data, uint32_t repeat=1){
    ESP_EARLY_LOGI(TAG, "Sending data to InfluxDB");
//    ESP_EARLY_LOGI(TAG, "DATA: %s", data.c_str());
    http.begin("http://insantr.od.ua:8086/write?db=weather");
    t_http_codes httpCode = (t_http_codes) http.POST(data);
    for(uint32_t left_try = repeat; (left_try > 0 || repeat == 0); left_try--){
        switch (httpCode){
            case t_http_codes::HTTP_CODE_NO_CONTENT: {
                ESP_EARLY_LOGI(TAG, "[HTTP] POST... code: %d", httpCode);
                return true;
            }
            default: {
                httpCode = (t_http_codes) http.POST(data);
                ESP_LOGE(TAG, "[HTTP] POST... code: %d; left_try: %d;", httpCode, left_try);
            };
        };
    }
    return (httpCode == HTTP_CODE_NO_CONTENT);
}
*/

/*
void storeData(std::vector <sens_data> data){
    Preferences _pref;
    _pref.begin("sensor", false);
    _pref.putUInt("array-size", data.size());
    _pref.putBytes("array-data", data.data(), (data.size() * sizeof(sens_data)));
    _pref.end();
}

std::vector <sens_data> loadData(){
    Preferences _pref;
    _pref.begin("sensor", true);
    uint32_t array_size = _pref.getUInt("array-size", 0);
    sens_data data[array_size];
    _pref.getBytes("array-data", data, sizeof(data));
    return std::vector <sens_data> (data, data + sizeof(data) / sizeof(data[0]));
}

bool clearData(){
    ESP_EARLY_LOGI(TAG, "clearData");
    Preferences _pref;
    _pref.begin("sensor", false);
    bool result = _pref.clear();
    ESP_EARLY_LOGI(TAG, "Clear data storeg %d", result);
    _pref.end();
    return result;
}*/


std::vector <sens_data> prepareData(bool isStore=true) {
    ESP_EARLY_LOGI(TAG, "Prepare data");
    bme.begin();

    float temp(NAN), hum(NAN), pres(NAN), volt(NAN), chip_temp(NAN);

    BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
    BME280::PresUnit presUnit(BME280::PresUnit_Pa);

    bme.read(pres, temp, hum, tempUnit, presUnit);

    chip_temp = temperatureRead();


    //////////////////
    int channel = digitalPinToAnalogChannel(34);
    adc_channel_t channel2 = adc_channel_t::ADC_CHANNEL_6;
    adc1_channel_t channel3 = adc1_channel_t::ADC1_CHANNEL_6;
    adc_unit_t unit = adc_unit_t::ADC_UNIT_1;
    adc_atten_t atten = adc_atten_t::ADC_ATTEN_DB_0;
    adc_bits_width_t wbit = adc_bits_width_t::ADC_WIDTH_BIT_9;
    uint32_t voltage= 0;
    adc1_config_width(wbit);
    adc1_config_channel_atten(channel3, atten);

    esp_adc_cal_characteristics_t characteristics;
    esp_adc_cal_get_characteristics(ADC_CAL_IDEAL_V_REF, atten, wbit, &characteristics);


    for (int i = 0; i < 100; i++){
        voltage += adc1_to_voltage(channel3, &characteristics);
        delay(50);
    }
    voltage /= 100;
    voltage *= 4.403;
    ///////////////////

    BME280::Settings settings = bme.getSettings();
    settings.mode = BME280::Mode_Sleep;
    bme.setSettings(settings);

    time_t _time_sec;
    time(&_time_sec);


    if(isStore){
        std::vector <sens_data> data = store.load();
        data.push_back(sens_data{ temp, hum, pres, voltage, chip_temp, _time_sec });
        store.save(data);
        return data;
    } else {
        std::vector <sens_data> data;
        data.push_back(sens_data{ temp, hum, pres, voltage, chip_temp, _time_sec });
        return data;
    }

}


void setup() {
    _init();


    ESP_EARLY_LOGI(TAG, "Setup");

    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();

    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_TIMER: {
            struct tm timeinfo;
            if (!getLocalTime(&timeinfo)) {
                init_ntp_client_and_update_time();
            }
            std::vector<sens_data> data = prepareData(false);
            if (data.size() >= 1) {
                if (connect_to_wifi(30)) {
                    if (influx.send(data)) {
                        store.clear();
                    }
                    WiFi.disconnect(true);
                    WiFi.mode(WIFI_OFF);
                }
            }
            go_to_sleep();
        }
            break;
        default: {
            store.clear();
            esp_chip_info_t chip_info;
            esp_chip_info(&chip_info);
            ESP_EARLY_LOGI(TAG,
                           "Chip rev number: %d; Cores: %d; Free Hap: %d bytes; Cycle: %d; SDK Ver: %s; "
                                   "FlashChipSize: %d byte; getFlashChipSpeed: %d Hz; FlashChipMode: %d;",
                           chip_info.revision, chip_info.cores, ESP.getFreeHeap(), ESP.getCycleCount(),
                           ESP.getSdkVersion(), ESP.getFlashChipSize(), ESP.getFlashChipSpeed(), ESP.getFlashChipMode())

            init_ntp_client_and_update_time();
            go_to_sleep(5);
        }
            break;
    }
}

void loop() { }
