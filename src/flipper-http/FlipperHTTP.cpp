/* FlipperHTTP.cpp for flipper-http.ino and FlipperHTTP.h
Author: JBlanked
Github: https://github.com/jblanked/FlipperHTTP
Info: This library is a wrapper around the HTTPClient library and is used to communicate with the FlipperZero over serial.
Created: 2024-09-30
Updated: 2026-06-04
*/

#include "FlipperHTTP.hpp"
#include "wifi_ap.hpp"
#include "wifi_deauth.hpp"
#include "common.hpp"
#include "command.hpp"

// Load WiFi settings
bool FlipperHTTP::loadWiFi()
{
    if (!storage.read(settingsFilePath, this->state))
    {
        this->uart->println(F("[ERROR] No settings saved yet or failed to read settings."));
        return false;
    }

    if (this->state.networkCount == 0)
    {
        this->uart->println(F("[ERROR] No WiFi networks saved."));
        return false;
    }

    // Try to connect to saved index first
    this->led.connecting();
    if (this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
    {
        char message[128];
        snprintf(message, sizeof(message), "WiFi connected to %s", this->state.networks[this->state.networkCurrent].ssid);
        this->uart->println(message);
        if (this->state.ledState) { this->led.connectedReady(); } else { this->led.off(); }
        return true;
    }
    this->led.connectFailed();

    // Try others
    for (uint8_t i = 0; i < this->state.networkCount; i++)
    {
        this->led.connecting();
        if (this->wifi.connect(this->state.networks[i].ssid, this->state.networks[i].pass))
        {
            this->state.networkCurrent = i;
            char message[128];
            snprintf(message, sizeof(message), "WiFi connected to %s", this->state.networks[this->state.networkCurrent].ssid);
            this->uart->println(message);
            if (this->state.ledState) { this->led.connectedReady(); } else { this->led.off(); }
            return true;
        }
        this->led.connectFailed();
    }

    this->uart->println(F("[ERROR] Failed to connect to any WiFi network."));
    return false;
}

// Save WiFi settings to storage
bool FlipperHTTP::saveWiFi(const char *newSSID, const char *newPassword)
{
    // check for duplicates
    for (uint8_t i = 0; i < this->state.networkCount; i++)
    {
        if (strcmp(this->state.networks[i].ssid, newSSID) == 0)
        {
            this->state.networkCurrent = i; // Set current network index to the existing one
            return true;
        }
    }

    snprintf(this->state.networks[this->state.networkCount].ssid, sizeof(this->state.networks[this->state.networkCount].ssid), "%s", newSSID);
    snprintf(this->state.networks[this->state.networkCount].pass, sizeof(this->state.networks[this->state.networkCount].pass), "%s", newPassword);
    this->state.networkCurrent = this->state.networkCount; // Set current network index to the new one
    this->state.networkCount++;                            // Increment network count

    if (!storage.write(settingsFilePath, state))
    {
        this->uart->println(F("[ERROR] Failed to write settings to storage."));
        return false;
    }

    return true;
}

void FlipperHTTP::setup()
{
    this->uart = new UART();
#ifdef BOARD_VGM
    this->uart->set_pins(0, 1);
#endif
    this->uart->begin(115200);
    this->uart->setTimeout(5000);
#if defined(BOARD_VGM)
    this->uart_2 = new UART();
    this->uart_2->set_pins(24, 21);
    this->uart_2->begin(115200);
    this->uart_2->setTimeout(5000);
    this->uart_2->flush();
#endif
    this->led.start();
    if (!storage.begin())
    {
        this->uart->println(F("[ERROR] Storage initialization failed."));
    }
    else
    {
        this->loadWiFi(); // Load WiFi settings
    }
    this->http = new HTTP(this->uart, &this->client);
    this->websocket = nullptr;
    this->uart->flush();
    this->led.off();
}

// Main loop for flipper-http.ino that handles all of the commands
void FlipperHTTP::loop()
{
#ifdef BOARD_VGM
    // Check if there's incoming serial data
    if (this->uart->available() > 0)
    {
        if (this->state.ledState)
        {
            this->led.activity();
        }

        // Read the incoming serial data until newline
        String _data = this->uart->readSerialLine();

        // send to ESP32
        this->uart_2->println(_data);

        // Wait for response from ESP32
        String _response = this->uart_2->readSerialLine();

        // Send response back to Flipper
        this->uart->println(_response);

        if (this->state.ledState)
        {
            this->led.off();
        }
    }
    else if (this->uart_2->available() > 0)
    {
        if (this->state.ledState)
        {
            this->led.activity();
        }

        // Read the incoming serial data until newline
        String _data = this->uart_2->readSerialLine();

        // send to Flipper
        this->uart->println(_data);

        if (this->state.ledState)
        {
            this->led.off();
        }
    }
#else
    // Check if there's incoming serial data
    if (this->uart->available())
    {
        // Read the incoming serial data until newline
        String _data = this->uart->readSerialLine();
        CommandType commandType = commandFromString(_data);
        if (commandType == COMMAND_TYPE_UNKNOWN)
        {
            return;
        }

        if (this->state.ledState)
        {
            this->led.activity();
        }

        switch (commandType)
        {
        case COMMAND_TYPE_LIST:
            this->uart->println(F("[LIST], [PING], [REBOOT], [WIFI/IP], [WIFI/SCAN], [WIFI/SAVE], [WIFI/CONNECT], [WIFI/DISCONNECT], [WIFI/LIST], [GET], [GET/HTTP], [POST/HTTP], [PUT/HTTP], [PATCH/HTTP], [DELETE/HTTP], [GET/BYTES], [POST/BYTES], [POST/FILE], [PARSE], [PARSE/ARRAY], [LED/ON], [LED/OFF], [IP/ADDRESS], [WIFI/AP], [VERSION], [DEAUTH], [WIFI/STATUS], [WIFI/SSID], [BOARD/NAME]"));
            break;
        case COMMAND_TYPE_PING:
            this->uart->println("[PONG]");
            break;
        case COMMAND_TYPE_REBOOT:
            this->uart->println(F("Rebooting..."));
            commonReboot();
            break;
        case COMMAND_TYPE_WIFI_IP:
        {
            if (!this->wifi.isConnected() && !this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
            {
                this->uart->println(F("[ERROR] Not connected to Wifi. Failed to reconnect."));
                this->led.off();
                return;
            }
            // Get Request
            String jsonData = this->http->request("GET", "https://httpbin.org/get");
            if (jsonData == "")
            {
                this->uart->println(F("[ERROR] GET request failed or returned empty data."));
                return;
            }
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);
            if (error)
            {
                this->uart->println(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }
            if (!doc["origin"])
            {
                this->uart->println(F("[ERROR] JSON does not contain origin."));
                this->led.off();
                return;
            }
            this->uart->println(doc["origin"].as<String>());
            this->uart->flush();
            this->uart->println();
            this->uart->println(F("[GET/END]"));
            break;
        }
        case COMMAND_TYPE_WIFI_SCAN:
        {
            if (this->state.ledState) { this->led.scanning(); }
            String scanResult = this->wifi.scan();
            if (this->state.ledState) { this->led.scanDone(scanResult.length() > 15); }
            this->uart->println(F("[GET/SUCCESS]"));
            this->uart->println(scanResult);
            this->uart->flush();
            this->uart->println();
            this->uart->println(F("[GET/END]"));
            break;
        }
        case COMMAND_TYPE_WIFI_SAVE:
        {
            // Extract JSON data by removing the command part
            String jsonData = _data.substring(strlen("[WIFI/SAVE]"));
            jsonData.trim(); // Remove any leading/trailing whitespace

            // Parse and save the settings
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON: "));
                this->uart->println(F(error.c_str()));
                return;
            }

            // Extract values from JSON
            if (!doc["ssid"] && doc["password"])
            {
                this->uart->println(F("[ERROR] JSON does not contain ssid and password."));
                return;
            }

            if (this->wifi.isConnected())
            {
                this->wifi.disconnect();
            }

            // Attempt to reconnect with new settings
            if (this->state.ledState) { this->led.connecting(); }
            if (this->wifi.connect(doc["ssid"], doc["password"]))
            {
                if (this->state.ledState) { this->led.connectedReady(); }
                // Save to storage
                if (!this->saveWiFi(doc["ssid"], doc["password"]))
                {
                    this->uart->println(F("[ERROR] Connected to WiFi but failed to save settings to file."));
                }
                else
                {
                    this->uart->println(F("[SUCCESS] WiFi settings saved and WiFi connected successfully."));
                }
                return;
            }
            else
            {
                if (this->state.ledState) { this->led.connectFailed(); }
                this->uart->println(F("[ERROR] Failed to connect to WiFi."));
                return;
            }
            break;
        }
        case COMMAND_TYPE_WIFI_CONNECT:
        {
            // Check if WiFi is already connected
            if (!this->wifi.isConnected())
            {
                // Attempt to connect to Wifi
                if (this->state.ledState) { this->led.connecting(); }
                if (this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
                {
                    if (this->state.ledState) { this->led.connectedReady(); }
                    this->uart->println(F("[SUCCESS] Connected to Wifi."));
                }
                else
                {
                    if (this->state.ledState) { this->led.connectFailed(); }
                    this->uart->println(F("[ERROR] Failed to connect to Wifi."));
                }
            }
            else
            {
                this->uart->println(F("[INFO] Already connected to WiFi."));
            }
            break;
        }
        case COMMAND_TYPE_WIFI_DISCONNECT:
        {
            this->wifi.disconnect();
            if (this->state.ledState) { this->led.disconnected(); }
            this->uart->println(F("[DISCONNECTED] WiFi has been disconnected."));
            break;
        }
        case COMMAND_TYPE_WIFI_LIST:
        {
            String fileContent = storage.read(settingsFilePath);
            this->uart->println(fileContent);
            this->uart->flush();
            break;
        }
        case COMMAND_TYPE_GET:
        {
            if (!this->wifi.isConnected() && !this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
            {
                this->uart->println(F("[ERROR] Not connected to WiFi. Failed to reconnect."));
                this->led.off();
                return;
            }
            // Extract URL by removing the command part
            String url = _data.substring(strlen("[GET]"));
            url.trim();

            // GET request
            String getData = this->http->request("GET", url);
            if (getData != "")
            {
                this->uart->println(getData);
                this->uart->flush();
                this->uart->println();
                this->uart->println(F("[GET/END]"));
            }
            else
            {
                this->uart->println(F("[ERROR] GET request failed or returned empty data."));
            }
            break;
        }
        case COMMAND_TYPE_GET_HTTP:
        {
            if (!this->wifi.isConnected() && !this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
            {
                this->uart->println(F("[ERROR] Not connected to Wifi. Failed to reconnect."));
                this->led.off();
                return;
            }

            // Extract the JSON by removing the command part
            String jsonData = _data.substring(strlen("[GET/HTTP]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Extract values from JSON
            if (!doc["url"])
            {
                this->uart->println(F("[ERROR] JSON does not contain url."));
                this->led.off();
                return;
            }
            String url = doc["url"];

            // Extract headers if available
            const char *headerKeys[10];
            const char *headerValues[10];
            int headerSize = 0;

            if (doc["headers"])
            {
                JsonObject headers = doc["headers"];
                for (JsonPair header : headers)
                {
                    headerKeys[headerSize] = header.key().c_str();
                    headerValues[headerSize] = header.value();
                    headerSize++;
                }
            }

            // GET request
            String getData = this->http->request("GET", url, "", headerKeys, headerValues, headerSize);
            if (getData != "")
            {
                this->uart->println(getData);
                this->uart->flush();
                this->uart->println();
                this->uart->println(F("[GET/END]"));
            }
            else
            {
                this->uart->println(F("[ERROR] GET request failed or returned empty data."));
            }
            break;
        }
        case COMMAND_TYPE_POST_HTTP:
        {
            if (!this->wifi.isConnected() && !this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
            {
                this->uart->println(F("[ERROR] Not connected to Wifi. Failed to reconnect."));
                this->led.off();
                return;
            }

            // Extract the JSON by removing the command part
            String jsonData = _data.substring(strlen("[POST/HTTP]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Extract values from JSON
            if (!doc["url"] || !doc["payload"])
            {
                this->uart->println(F("[ERROR] JSON does not contain url or payload."));
                this->led.off();
                return;
            }
            String url = doc["url"];
            String payload = doc["payload"];

            // Extract headers if available
            const char *headerKeys[10];
            const char *headerValues[10];
            int headerSize = 0;

            if (doc["headers"])
            {
                JsonObject headers = doc["headers"];
                for (JsonPair header : headers)
                {
                    headerKeys[headerSize] = header.key().c_str();
                    headerValues[headerSize] = header.value();
                    headerSize++;
                }
            }

            // POST request
            String postData = this->http->request("POST", url, payload, headerKeys, headerValues, headerSize);
            if (postData != "")
            {
                this->uart->println(postData);
                this->uart->flush();
                this->uart->println();
                this->uart->println(F("[POST/END]"));
            }
            else
            {
                this->uart->println(F("[ERROR] POST request failed or returned empty data."));
            }
            break;
        }
        case COMMAND_TYPE_PUT_HTTP:
        {
            if (!this->wifi.isConnected() && !this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
            {
                this->uart->println(F("[ERROR] Not connected to Wifi. Failed to reconnect."));
                this->led.off();
                return;
            }

            // Extract the JSON by removing the command part
            String jsonData = _data.substring(strlen("[PUT/HTTP]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Extract values from JSON
            if (!doc["url"] || !doc["payload"])
            {
                this->uart->println(F("[ERROR] JSON does not contain url or payload."));
                this->led.off();
                return;
            }
            String url = doc["url"];
            String payload = doc["payload"];

            // Extract headers if available
            const char *headerKeys[10];
            const char *headerValues[10];
            int headerSize = 0;

            if (doc["headers"])
            {
                JsonObject headers = doc["headers"];
                for (JsonPair header : headers)
                {
                    headerKeys[headerSize] = header.key().c_str();
                    headerValues[headerSize] = header.value();
                    headerSize++;
                }
            }

            // PUT request
            String putData = this->http->request("PUT", url, payload, headerKeys, headerValues, headerSize);
            if (putData != "")
            {
                this->uart->println(putData);
                this->uart->flush();
                this->uart->println();
                this->uart->println(F("[PUT/END]"));
            }
            else
            {
                this->uart->println(F("[ERROR] PUT request failed or returned empty data."));
            }
            break;
        }
        case COMMAND_TYPE_PATCH_HTTP:
        {
            if (!this->wifi.isConnected() && !this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
            {
                this->uart->println(F("[ERROR] Not connected to Wifi. Failed to reconnect."));
                this->led.off();
                return;
            }

            String jsonData = _data.substring(strlen("[PATCH/HTTP]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            if (!doc["url"] || !doc["payload"])
            {
                this->uart->println(F("[ERROR] JSON does not contain url or payload."));
                this->led.off();
                return;
            }
            String url = doc["url"];
            String payload = doc["payload"];

            const char *headerKeys[10];
            const char *headerValues[10];
            int headerSize = 0;

            if (doc["headers"])
            {
                JsonObject headers = doc["headers"];
                for (JsonPair header : headers)
                {
                    headerKeys[headerSize] = header.key().c_str();
                    headerValues[headerSize] = header.value();
                    headerSize++;
                }
            }

            String patchData = this->http->request("PATCH", url, payload, headerKeys, headerValues, headerSize);
            if (patchData != "")
            {
                this->uart->println(patchData);
                this->uart->flush();
                this->uart->println();
                this->uart->println(F("[PATCH/END]"));
            }
            else
            {
                this->uart->println(F("[ERROR] PATCH request failed or returned empty data."));
            }
            break;
        }
        case COMMAND_TYPE_DELETE_HTTP:
        {
            if (!this->wifi.isConnected() && !this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
            {
                this->uart->println(F("[ERROR] Not connected to Wifi. Failed to reconnect."));
                this->led.off();
                return;
            }

            // Extract the JSON by removing the command part
            String jsonData = _data.substring(strlen("[DELETE/HTTP]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Extract values from JSON
            if (!doc["url"] || !doc["payload"])
            {
                this->uart->println(F("[ERROR] JSON does not contain url or payload."));
                this->led.off();
                return;
            }
            String url = doc["url"];
            String payload = doc["payload"];

            // Extract headers if available
            const char *headerKeys[10];
            const char *headerValues[10];
            int headerSize = 0;

            if (doc["headers"])
            {
                JsonObject headers = doc["headers"];
                for (JsonPair header : headers)
                {
                    headerKeys[headerSize] = header.key().c_str();
                    headerValues[headerSize] = header.value();
                    headerSize++;
                }
            }

            // DELETE request
            String deleteData = this->http->request("DELETE", url, payload, headerKeys, headerValues, headerSize);
            if (deleteData != "")
            {
                this->uart->println(deleteData);
                this->uart->flush();
                this->uart->println();
                this->uart->println(F("[DELETE/END]"));
            }
            else
            {
                this->uart->println(F("[ERROR] DELETE request failed or returned empty data."));
            }
            break;
        }
        case COMMAND_TYPE_GET_BYTES:
        {
            if (!this->wifi.isConnected() && !this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
            {
                this->uart->println(F("[ERROR] Not connected to Wifi. Failed to reconnect."));
                this->led.off();
                return;
            }

            // Extract the JSON by removing the command part
            String jsonData = _data.substring(strlen("[GET/BYTES]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Extract values from JSON
            if (!doc["url"])
            {
                this->uart->println(F("[ERROR] JSON does not contain url."));
                this->led.off();
                return;
            }
            String url = doc["url"];

            // Extract headers if available
            const char *headerKeys[10];
            const char *headerValues[10];
            int headerSize = 0;

            if (doc["headers"])
            {
                JsonObject headers = doc["headers"];
                for (JsonPair header : headers)
                {
                    headerKeys[headerSize] = header.key().c_str();
                    headerValues[headerSize] = header.value();
                    headerSize++;
                }
            }

            // GET request
            if (!this->http->stream("GET", url, "", headerKeys, headerValues, headerSize))
            {
                this->uart->println(F("[ERROR] GET request failed or returned empty data."));
            }
            break;
        }
        case COMMAND_TYPE_POST_BYTES:
        {
            if (!this->wifi.isConnected() && !this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
            {
                this->uart->println(F("[ERROR] Not connected to Wifi. Failed to reconnect."));
                this->led.off();
                return;
            }

            // Extract the JSON by removing the command part
            String jsonData = _data.substring(strlen("[POST/BYTES]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Extract values from JSON
            if (!doc["url"] || !doc["payload"])
            {
                this->uart->println(F("[ERROR] JSON does not contain url or payload."));
                this->led.off();
                return;
            }
            String url = doc["url"];
            String payload = doc["payload"];

            // Extract headers if available
            const char *headerKeys[10];
            const char *headerValues[10];
            int headerSize = 0;

            if (doc["headers"])
            {
                JsonObject headers = doc["headers"];
                for (JsonPair header : headers)
                {
                    headerKeys[headerSize] = header.key().c_str();
                    headerValues[headerSize] = header.value();
                    headerSize++;
                }
            }

            // POST request
            if (!this->http->stream("POST", url, payload, headerKeys, headerValues, headerSize))
            {
                this->uart->println(F("[ERROR] POST request failed or returned empty data."));
            }
            break;
        }
        case COMMAND_TYPE_POST_FILE:
        {
            if (!this->wifi.isConnected() && !this->wifi.connect(this->state.networks[this->state.networkCurrent].ssid, this->state.networks[this->state.networkCurrent].pass))
            {
                this->uart->println(F("[ERROR] Not connected to Wifi. Failed to reconnect."));
                this->led.off();
                return;
            }

            // Extract the JSON by removing the command part
            String jsonData = _data.substring(strlen("[POST/FILE]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Require url and size; content_type is optional (defaults to application/octet-stream)
            if (!doc["url"] || !doc["size"])
            {
                this->uart->println(F("[ERROR] JSON does not contain url or size."));
                this->led.off();
                return;
            }
            String url = doc["url"];
            size_t fileSize = doc["size"].as<size_t>();
            String contentType = doc["content_type"] | "application/octet-stream";

            // Extract optional headers
            const char *headerKeys[10];
            const char *headerValues[10];
            int headerSize = 0;

            if (doc["headers"])
            {
                JsonObject headers = doc["headers"];
                for (JsonPair header : headers)
                {
                    headerKeys[headerSize] = header.key().c_str();
                    headerValues[headerSize] = header.value();
                    headerSize++;
                }
            }

            // Upload: connect, stream bytes from UART to HTTP body, return response
            if (!this->http->streamUpload("POST", url, fileSize, contentType, headerKeys, headerValues, headerSize))
            {
                this->uart->println(F("[ERROR] File upload failed."));
            }
            break;
        }
        case COMMAND_TYPE_PARSE:
        {
            // Extract the JSON by removing the command part
            String jsonData = _data.substring(strlen("[PARSE]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Extract values from JSON
            if (!doc["key"] || !doc["json"])
            {
                this->uart->println(F("[ERROR] JSON does not contain key or json."));
                this->led.off();
                return;
            }
            String key = doc["key"];
            JsonObject json = doc["json"];

            if (json[key])
            {
                this->uart->println(json[key].as<String>());
            }
            else
            {
                this->uart->println(F("[ERROR] Key not found in JSON."));
            }
            break;
        }
        case COMMAND_TYPE_PARSE_ARRAY:
        {
            // Extract the JSON by removing the command part
            String jsonData = _data.substring(strlen("[PARSE/ARRAY]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Extract values from JSON
            if (!doc["key"] || !doc["index"] || !doc["json"])
            {
                this->uart->println(F("[ERROR] JSON does not contain key, index, or json."));
                this->led.off();
                return;
            }
            String key = doc["key"];
            int index = doc["index"];
            JsonArray json = doc["json"];

            if (json[index][key])
            {
                this->uart->println(json[index][key].as<String>());
            }
            else
            {
                this->uart->println(F("[ERROR] Key not found in JSON."));
            }
            break;
        }
        case COMMAND_TYPE_LED_ON:
            this->state.ledState = true;
            if (storage.write(settingsFilePath, state))
            {
                this->uart->println(F("[SUCCESS] LED enabled and state saved."));
            }
            else
            {
                this->uart->println(F("[ERROR] LED enabled but failed to save state."));
            }
            break;
        case COMMAND_TYPE_LED_OFF:
            this->state.ledState = false;
            if (storage.write(settingsFilePath, state))
            {
                this->uart->println(F("[SUCCESS] LED disabled and state saved."));
            }
            else
            {
                this->uart->println(F("[ERROR] LED disabled but failed to save state."));
            }
            break;
        case COMMAND_TYPE_IP_ADDRESS:
            this->uart->println(this->wifi.deviceIP());
            break;
        case COMMAND_TYPE_WIFI_AP:
        {
            // Extract the JSON by removing the command part
            String jsonData = _data.substring(strlen("[WIFI/AP]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Extract values from JSON
            if (!doc["ssid"])
            {
                this->uart->println(F("[ERROR] JSON does not contain ssid."));
                this->led.off();
                return;
            }

            String ssid = doc["ssid"];

            WiFiAP ap(this->uart, &this->wifi);

            if (!ap.start(ssid.c_str()))
            {
                this->led.off();
                return; // error is handled by class
            }

            this->uart->println(F("[AP/CONNECTED]"));
            ap.run();
            this->uart->println(F("[AP/DISCONNECTED]"));
            break;
        }
        case COMMAND_TYPE_VERSION:
            this->uart->println(FLIPPER_HTTP_VERSION);
            break;
        case COMMAND_TYPE_DEAUTH:
        {
            // Extract the JSON by removing the command part
            String jsonData = _data.substring(strlen("[DEAUTH]"));
            jsonData.trim();

            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->print(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Extract values from JSON
            if (!doc["ssid"])
            {
                this->uart->println(F("[ERROR] JSON does not contain ssid"));
                this->led.off();
                return;
            }

            String ssid = doc["ssid"];

            WiFiDeauth deauther;
            this->uart->println(F("[DEAUTH/STARTING]"));

            if (!deauther.start(ssid.c_str()))
            {
                this->led.off();
                return; // error is handled by class
            }

            this->uart->println(F("[DEAUTH/STARTED]"));

            String uartMessage = "";
            while (uartMessage != "[DEAUTH/STOP]")
            {
                // Check if there's incoming serial data
                if (this->uart->available() > 0)
                {
                    // Read the incoming serial data until newline
                    uartMessage = this->uart->readSerialLine();
                }
                deauther.update();
            }
            deauther.stop();
            this->uart->println(F("[DEAUTH/STOPPED]"));
            break;
        }
        case COMMAND_TYPE_DEAUTH_STOP:
            // nothing to do..
            break;
        case COMMAND_TYPE_WIFI_STATUS:
            if (this->wifi.isConnected())
            {
                this->uart->println(F("true"));
            }
            else
            {
                this->uart->println(F("false"));
            }
            break;
        case COMMAND_TYPE_WIFI_SSID:
        {
            String ssid = this->wifi.getSSID();
            if (ssid != "")
            {
                this->uart->println(ssid);
            }
            else
            {
                this->uart->println(F("[ERROR] Not connected to WiFi."));
            }
            break;
        }
        case COMMAND_TYPE_BOARD_NAME:
            this->uart->println(commonGetBoardName());
            break;
        case COMMAND_TYPE_SOCKET_START:
        {
            // Remove the command prefix to isolate the JSON payload
            String jsonData = _data.substring(strlen("[SOCKET/START]"));
            jsonData.trim();

            // Create a JsonDocument with an appropriate size
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, jsonData);

            if (error)
            {
                this->uart->println(F("[ERROR] Failed to parse JSON."));
                this->led.off();
                return;
            }

            // Ensure that the JSON contains a "url" and "port"
            if (!doc["url"])
            {
                this->uart->println(F("[ERROR] JSON does not contain url."));
                this->led.off();
                return;
            }
            String fullUrl = doc["url"].as<String>();

            if (!doc["port"])
            {
                this->uart->println(F("[ERROR] JSON does not contain port."));
                this->led.off();
                return;
            }
            int port = doc["port"].as<int>();

            // Parse the fullUrl to extract server name and path.
            // Expected format: "ws://www.jblanked.com/ws/game/new/"
            String serverName;
            String path = "/";

            // Remove protocol ("ws://" or "wss://")
            if (fullUrl.startsWith("ws://"))
            {
                fullUrl = fullUrl.substring(5);
            }
            else if (fullUrl.startsWith("wss://"))
            {
                fullUrl = fullUrl.substring(6);
            }

            // Look for the first '/' that separates the server name from the path.
            int slashIndex = fullUrl.indexOf('/');
            if (slashIndex != -1)
            {
                serverName = fullUrl.substring(0, slashIndex);
                path = fullUrl.substring(slashIndex);
            }
            else
            {
                serverName = fullUrl;
                path = "/";
            }

            // Extract headers if available
            int headerSize = 0;
            const char *headerKeys[10];
            const char *headerValues[10];

            if (doc["headers"])
            {
                JsonObject headers = doc["headers"];
                for (JsonPair kv : headers)
                {
                    headerKeys[headerSize] = kv.key().c_str();
                    headerValues[headerSize] = kv.value().as<const char *>();
                    headerSize++;
                }
            }

            if (!this->websocket)
            {
                this->websocket = new WebSocket();
            }

            if (!this->websocket)
            {
                this->uart->println(F("[ERROR] Failed to allocate WebSocket object."));
                this->led.off();
                return;
            }

            if (!this->websocket->connect(serverName.c_str(), port, path.c_str(), headerKeys, headerValues, headerSize))
            {
                char headerResponse[128];
                snprintf(headerResponse, sizeof(headerResponse), "[ERROR] Failed to connect WebSocket to %s:%d%s", serverName.c_str(), port, path.c_str());
                this->uart->println(headerResponse);
                this->led.off();
                return;
            }

            this->uart->println(F("[SOCKET/CONNECTED]"));

            delay(100);

            // Check if a message is available from the server:
            String receivedMessage = this->websocket->recv();
            if (this->websocket->isConnected() && receivedMessage.length() > 0)
            {
                // Read the message from the server
                this->uart->println(receivedMessage);
            }

            // Wait for incoming serial/client data, and send back-n-forth
            String uartMessage = "";
            String wsMessage = "";
            while (this->websocket->isConnected())
            {
                // Check if there's incoming serial data
                if (this->uart->available() > 0)
                {
                    // Read the incoming serial data until newline
                    uartMessage = this->uart->readSerialLine();
                    if (uartMessage.startsWith("[SOCKET/STOP]"))
                    {
                        break;
                    }
                    this->websocket->send(uartMessage);
                    uartMessage = ""; // Clear the message after sending
                }

                // Check if there's incoming websocket data
                wsMessage = this->websocket->recv();
                if (wsMessage.length() > 0)
                {
                    // Read the message from the server
                    this->uart->println(wsMessage);
                    wsMessage = ""; // Clear the message after printing
                }
            }

            // Close the WebSocket connection
            this->websocket->stop();
            this->uart->println(F("[SOCKET/STOPPED]"));
            break;
        }
        case COMMAND_TYPE_SOCKET_STOP:
            // nothing to do..
            break;
        default:
            break;
        }

        if (this->state.ledState)
        {
            this->led.off();
        }
    }
    // Heartbeat: brief pulse while connected
    if (this->wifi.isConnected() && this->state.ledState)
    {
        this->led.connectedTick();
    }
#endif
}
