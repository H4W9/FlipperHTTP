#pragma once
#include "boards.hpp"
#if defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
#include <LittleFS.h>
#elif defined(BOARD_BW16)
#include <FlashStorage_RTL8720.h> // https://github.com/khoih-prog/FlashStorage_RTL8720
#else
#include <SPIFFS.h>
#endif
#include <ArduinoJson.h>

typedef struct
{
    char ssid[64];
    char pass[64];
} network_t;

typedef struct
{
    uint8_t networkCount;
    uint8_t networkCurrent;
    network_t networks[32];
    uint8_t ledState;
} state_t;

class StorageManager
{
public:
    bool begin();
    bool deserialize(JsonDocument &doc, const char *filename);
    String read(const char *filename);
    bool read(const char *filename, state_t &state);
    bool serialize(JsonDocument &doc, const char *filename);
    bool write(const char *filename, const char *data);
    bool write(const char *filename, const state_t &state);
};