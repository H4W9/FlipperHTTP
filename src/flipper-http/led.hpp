#pragma once
#include <Arduino.h>
#include "boards.hpp"

class LED
{
public:
    LED()
    {
    }
    void blink(int timeout = 250); // Turn on and off the LED
    void start();                  // Display LED sequence
    void on();                     // Turn on LED (Green only)
    void off();                    // Turn off all LEDs

    // ── Rich RGB status indicators (ESP32-S3 and ESP32-C5 only) ──────────────
    // bootSequence() always plays at boot regardless of the LED ON/OFF setting.
    // All other methods must be gated by use_led in the caller.
    void bootSequence();            // R->G->B->white flash->3x white blinks  (boot only)
    void activity();                // Cyan on        — command received / processing
    void scanning();                // Blue steady    — WiFi scan in progress
    void connecting();              // Yellow steady  — joining a network
    void connectedReady();          // Green 2 s -> off — just connected
    void connectedTick();           // Non-blocking: green 600 ms pulse every 30 s (call from loop)
    void disconnected();            // Red 400ms->off — link dropped
    void connectFailed();           // Red 1s->off    — connection attempt failed
    void scanDone(bool found);      // Yellow 300ms->off (found) / Red 600ms->off (none)
};
