#include "storage.hpp"

#define UNUSED(expr)  \
    do                \
    {                 \
        (void)(expr); \
    } while (0)

bool StorageManager::begin()
{
#if defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    if (!LittleFS.begin())
    {
        if (LittleFS.format())
        {
            if (!LittleFS.begin())
            {
                return false;
            }
        }
        return false;
    }
    return true;
#elif defined(BOARD_BW16)
    // no begin needed
    return true;
#else
    return SPIFFS.begin(true);
#endif
}

bool StorageManager::deserialize(JsonDocument &doc, const char *filename)
{
#if defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    File file = LittleFS.open(filename, "r");
#elif !defined(BOARD_BW16)
    File file = SPIFFS.open(filename, FILE_READ);
#endif
#ifndef BOARD_BW16
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    return !error; // return no error
#else
    /*We will keep all data at the same address and overwrite for now*/
    UNUSED(filename);
    char buffer[512];
    FlashStorage.get(0, buffer);
    buffer[sizeof(buffer) - 1] = '\0'; // Null-terminate the string
    DeserializationError error = deserializeJson(doc, buffer);
    return !error; // return no error
#endif
}

String StorageManager::read(const char *filename)
{
    String fileContent = "";
#if defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    File file = LittleFS.open(filename, "r");
#elif !defined(BOARD_BW16)
    File file = SPIFFS.open(filename, FILE_READ);
#endif
#ifndef BOARD_BW16
    if (file)
    {
        // Read the entire file content
        fileContent = file.readString();
        file.close();
    }
#else
    /*We will keep all data at the same address and overwrite for now*/
    // we wrote in char so we need to read in char, then convert to string
    UNUSED(filename);
    char buffer[512];
    FlashStorage.get(0, buffer);
    buffer[sizeof(buffer) - 1] = '\0'; // Null-terminate the string
    fileContent = String(buffer);
#endif
    return fileContent;
}

bool StorageManager::read(const char *filename, state_t &state)
{
    size_t bytesRead = 0;
#if defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    File file = LittleFS.open(filename, "rb");
#elif !defined(BOARD_BW16)
    File file = SPIFFS.open(filename, "rb");
#endif
#ifndef BOARD_BW16
    if (file)
    {
        bytesRead = file.readBytes((char *)&state, sizeof(state_t));
        file.close();
    }
#else
    /*We will keep all data at the same address and overwrite for now*/
    UNUSED(filename);
    FlashStorage.get(0, (uint8_t *)&state);
#endif
    return bytesRead == sizeof(state_t);
}

bool StorageManager::serialize(JsonDocument &doc, const char *filename)
{
#if defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    File file = LittleFS.open(filename, "w");
#elif !defined(BOARD_BW16)
    File file = SPIFFS.open(filename, FILE_WRITE);
#endif
#ifndef BOARD_BW16
    if (file)
    {
        serializeJson(doc, file);
        file.close();
        return true;
    }
    return false;
#else
    /*We will keep all data at the same address and overwrite for now*/
    UNUSED(filename);
    char buffer[512];
    size_t len = serializeJson(doc, buffer, sizeof(buffer));
    buffer[len] = '\0'; // Null-terminate the string
    FlashStorage.put(0, buffer);
    return true;
#endif
}

bool StorageManager::write(const char *filename, const char *data)
{
#if defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    File file = LittleFS.open(filename, "w");
#elif !defined(BOARD_BW16)
    File file = SPIFFS.open(filename, FILE_WRITE);
#endif

#ifndef BOARD_BW16
    if (file)
    {
        file.print(data);
        file.close();
        return true;
    }
    return false;
#else
    /*We will keep all data at the same address and overwrite for now*/
    UNUSED(filename);
    FlashStorage.put(0, data);
    return true;
#endif
}

bool StorageManager::write(const char *filename, const state_t &state)
{
#if defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    File file = LittleFS.open(filename, "wb");
#elif !defined(BOARD_BW16)
    File file = SPIFFS.open(filename, "wb");
#endif

#ifndef BOARD_BW16
    if (file)
    {
        file.write((const uint8_t *)&state, sizeof(state_t));
        file.close();
        return true;
    }
    return false;
#else
    /*We will keep all data at the same address and overwrite for now*/
    UNUSED(filename);
    FlashStorage.put(0, (const uint8_t *)&state);
    return true;
#endif
}
