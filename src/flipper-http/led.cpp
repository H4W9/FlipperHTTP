#include "led.hpp"

#if defined(BOARD_ESP32_C6) || defined(BOARD_ESP32_C3) || defined(BOARD_ESP32_C5) || defined(BOARD_CARDPUTER)
#include <Adafruit_NeoPixel.h>
#if defined(BOARD_ESP32_C6)
constexpr uint8_t LED_PIN = 10;
#elif defined(BOARD_ESP32_C3)
constexpr uint8_t LED_PIN = 8;
#elif defined(BOARD_ESP32_C5)
constexpr uint8_t LED_PIN = 27;
#elif defined(BOARD_CARDPUTER)
constexpr uint8_t LED_PIN = 21;
#endif
constexpr uint8_t NUM_LEDS = 1;

extern Adafruit_NeoPixel rgbLed;

struct RGB
{
    uint8_t r, g, b;
};

constexpr RGB COLOR_OFF = {0, 0, 0};
constexpr RGB COLOR_GREEN = {0, 255, 0};
#endif

#if defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_BW16) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
#define LED_ON HIGH
#define LED_OFF LOW
#else
#define LED_ON LOW
#define LED_OFF HIGH
#endif

#if defined(BOARD_ESP32_C6) || defined(BOARD_ESP32_C5) || defined(BOARD_CARDPUTER)
Adafruit_NeoPixel rgbLed(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
#endif

// ── NeoPixel helpers — S3 and C5 only ────────────────────────────────────────
// Brightness kept low (40/255) to stay within 3V3 budget, matching cyberbrick.
#if defined(BOARD_ESP32_S3) || defined(BOARD_ESP32_C5)
constexpr uint8_t NEO_BRIGHT = 40;

static void neoSet(uint8_t r, uint8_t g, uint8_t b)
{
#if defined(BOARD_ESP32_S3)
#ifdef RGB_BUILTIN
    neopixelWrite(RGB_BUILTIN, r, g, b);
#endif
#elif defined(BOARD_ESP32_C5)
    rgbLed.setPixelColor(0, rgbLed.Color(r, g, b));
    rgbLed.show();
#endif
}

static inline void neoOff()    { neoSet(0,                  0,                  0          ); }
static inline void neoRed()    { neoSet(NEO_BRIGHT,          0,                  0          ); }
static inline void neoGreen()  { neoSet(0,                  NEO_BRIGHT,          0          ); }
static inline void neoBlue()   { neoSet(0,                  0,                  NEO_BRIGHT ); }
static inline void neoYellow() { neoSet(NEO_BRIGHT,          NEO_BRIGHT / 2,     0          ); }
static inline void neoWhite()  { neoSet(NEO_BRIGHT,          NEO_BRIGHT,         NEO_BRIGHT ); }

// State for non-blocking connectedTick()
// Phase 0 = idle, 1 = fade-in, 2 = fade-out
static uint32_t _conn_last_pulse  = 0;
static uint8_t  _conn_phase       = 0;
static uint32_t _conn_phase_start = 0;
constexpr uint32_t FADE_HALF_MS   = 500; // each half (in + out = 1000 ms total)
#endif // BOARD_ESP32_S3 || BOARD_ESP32_C5

// ── blink ─────────────────────────────────────────────────────────────────────
void LED::blink(int timeout)
{
#ifdef BOARD_WIFI_DEV
    digitalWrite(6, LED_OFF); // RED
    digitalWrite(4, LED_OFF); // BLUE
    digitalWrite(5, LED_ON);  // GREEN
    delay(timeout);
    digitalWrite(5, LED_OFF); // GREEN
    delay(timeout);
#elif defined(BOARD_ESP32_C6)
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_GREEN.r, COLOR_GREEN.g, COLOR_GREEN.b));
    rgbLed.show();
    delay(timeout);
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_OFF.r, COLOR_OFF.g, COLOR_OFF.b));
    rgbLed.show();
    delay(timeout);
#elif defined(BOARD_ESP32_CAM)
    digitalWrite(4, LED_ON);
    delay(timeout);
    digitalWrite(4, LED_OFF);
    delay(timeout);
#elif defined(BOARD_ESP32_S3)
#ifdef RGB_BUILTIN
    neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0);
    delay(timeout);
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
    delay(timeout);
#endif
#elif defined(BOARD_ESP32_WROOM)
    digitalWrite(2, LED_ON);
    delay(timeout);
    digitalWrite(2, LED_OFF);
    delay(timeout);
#elif defined(BOARD_ESP32_WROVER)
    digitalWrite(6, LED_OFF); // RED
    digitalWrite(4, LED_OFF); // BLUE
    digitalWrite(5, LED_ON);  // GREEN
    delay(timeout);
    digitalWrite(5, LED_OFF);
    delay(timeout);
#elif defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    digitalWrite(LED_BUILTIN, LED_ON);
    delay(timeout);
    digitalWrite(LED_BUILTIN, LED_OFF);
    delay(timeout);
#elif defined(BOARD_ESP32_C3)
    digitalWrite(8, LED_ON);
    delay(timeout);
    digitalWrite(8, LED_OFF);
    delay(timeout);
#elif defined(BOARD_BW16)
    digitalWrite(LED_B, LED_ON);
    delay(timeout);
    digitalWrite(LED_B, LED_OFF);
    delay(timeout);
#elif defined(BOARD_ESP32_C5)
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_GREEN.r, COLOR_GREEN.g, COLOR_GREEN.b));
    rgbLed.show();
    delay(timeout);
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_OFF.r, COLOR_OFF.g, COLOR_OFF.b));
    rgbLed.show();
    delay(timeout);
#elif defined(BOARD_CARDPUTER)
    // turn on GREEN
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_GREEN.r, COLOR_GREEN.g, COLOR_GREEN.b));
    rgbLed.show();
    delay(timeout);
    // turn off GREEN
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_OFF.r, COLOR_OFF.g, COLOR_OFF.b));
    rgbLed.show();
    delay(timeout);
#endif
}

// ── start ─────────────────────────────────────────────────────────────────────
void LED::start()
{
#ifdef BOARD_WIFI_DEV
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    digitalWrite(4, LED_OFF);
    digitalWrite(6, LED_OFF);
#elif defined(BOARD_ESP32_C6)
    rgbLed.begin();
    rgbLed.show();
#elif defined(BOARD_ESP32_CAM)
    pinMode(4, OUTPUT);
#elif defined(BOARD_ESP32_S3)
#ifdef RGB_BUILTIN
    digitalWrite(RGB_BUILTIN, LOW);
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
#endif
#elif defined(BOARD_ESP32_WROOM)
    pinMode(2, OUTPUT);
#elif defined(BOARD_ESP32_WROVER)
    pinMode(4, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(6, OUTPUT);
    digitalWrite(4, LED_OFF);
    digitalWrite(6, LED_OFF);
    digitalWrite(5, LED_OFF);
#elif defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    pinMode(LED_BUILTIN, OUTPUT);
#elif defined(BOARD_ESP32_C3)
    pinMode(8, OUTPUT);
#elif defined(BOARD_BW16)
    pinMode(LED_B, OUTPUT);
#elif defined(BOARD_ESP32_C5)
    rgbLed.begin();
    rgbLed.setBrightness(NEO_BRIGHT);
    rgbLed.show();
#elif defined(BOARD_CARDPUTER)
    rgbLed.begin();
    rgbLed.show();
#endif

    // S3 and C5 play the full RGB boot sequence; all other boards do 3x green blinks.
#if defined(BOARD_ESP32_S3) || defined(BOARD_ESP32_C5)
    this->bootSequence();
#else
    this->blink();
    this->blink();
    this->blink();
#endif
}

// ── on ───────────────────────────────────────────────────────────────────────
void LED::on()
{
#ifdef BOARD_WIFI_DEV
    digitalWrite(4, LED_OFF);
    digitalWrite(6, LED_OFF);
    digitalWrite(5, LED_ON);
#elif defined(BOARD_ESP32_C6)
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_GREEN.r, COLOR_GREEN.g, COLOR_GREEN.b));
    rgbLed.show();
#elif defined(BOARD_ESP32_CAM)
    digitalWrite(4, LED_ON);
#elif defined(BOARD_ESP32_S3)
#ifdef RGB_BUILTIN
    neopixelWrite(RGB_BUILTIN, 0, RGB_BRIGHTNESS, 0);
#endif
#elif defined(BOARD_ESP32_WROOM)
    digitalWrite(2, LED_ON);
#elif defined(BOARD_ESP32_WROVER)
    digitalWrite(4, LED_OFF);
    digitalWrite(6, LED_OFF);
    digitalWrite(5, LED_ON);
#elif defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    digitalWrite(LED_BUILTIN, LED_ON);
#elif defined(BOARD_ESP32_C3)
    digitalWrite(8, LED_ON);
#elif defined(BOARD_BW16)
    digitalWrite(LED_B, LED_ON);
#elif defined(BOARD_ESP32_C5)
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_GREEN.r, COLOR_GREEN.g, COLOR_GREEN.b));
    rgbLed.show();
#elif defined(BOARD_CARDPUTER)
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_GREEN.r, COLOR_GREEN.g, COLOR_GREEN.b));
    rgbLed.show();
#endif
}

// ── off ──────────────────────────────────────────────────────────────────────
void LED::off()
{
#ifdef BOARD_WIFI_DEV
    digitalWrite(4, LED_OFF);
    digitalWrite(5, LED_OFF);
    digitalWrite(6, LED_OFF);
#elif defined(BOARD_ESP32_C6)
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_OFF.r, COLOR_OFF.g, COLOR_OFF.b));
    rgbLed.show();
#elif defined(BOARD_ESP32_CAM)
    digitalWrite(4, LED_OFF);
#elif defined(BOARD_ESP32_S3)
#ifdef RGB_BUILTIN
    neopixelWrite(RGB_BUILTIN, 0, 0, 0);
#endif
#elif defined(BOARD_ESP32_WROOM)
    digitalWrite(2, LED_OFF);
#elif defined(BOARD_ESP32_WROVER)
    digitalWrite(4, LED_OFF);
    digitalWrite(5, LED_OFF);
    digitalWrite(6, LED_OFF);
#elif defined(BOARD_PICO_W) || defined(BOARD_PICO_2W) || defined(BOARD_VGM) || defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    digitalWrite(LED_BUILTIN, LED_OFF);
#elif defined(BOARD_ESP32_C3)
    digitalWrite(8, LED_OFF);
#elif defined(BOARD_BW16)
    digitalWrite(LED_B, LED_OFF);
#elif defined(BOARD_ESP32_C5)
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_OFF.r, COLOR_OFF.g, COLOR_OFF.b));
    rgbLed.show();
#elif defined(BOARD_CARDPUTER)
    rgbLed.setPixelColor(0, rgbLed.Color(COLOR_OFF.r, COLOR_OFF.g, COLOR_OFF.b));
    rgbLed.show();
#endif
}

// ── bootSequence ─────────────────────────────────────────────────────────────
// Mirrors led_boot_sequence() from cyberbrick_esp32.ino.
// Wipe R -> G -> B -> white, then three quick white blinks.
// Always runs at boot; not gated by use_led.
void LED::bootSequence()
{
#if defined(BOARD_ESP32_S3) || defined(BOARD_ESP32_C5)
    const uint8_t seq[4][3] = {
        {NEO_BRIGHT, 0,           0          },
        {0,          NEO_BRIGHT,  0          },
        {0,          0,           NEO_BRIGHT },
        {NEO_BRIGHT, NEO_BRIGHT,  NEO_BRIGHT },
    };
    for (int i = 0; i < 4; i++)
    {
        neoSet(seq[i][0], seq[i][1], seq[i][2]);
        delay(120);
    }
    for (int i = 0; i < 3; i++)
    {
        neoWhite(); delay(80);
        neoOff();   delay(80);
    }
#endif
}

// ── activity ─────────────────────────────────────────────────────────────────
// Cyan on — shown while a command is being processed.
// Falls back to plain green on non-NeoPixel boards via on().
void LED::activity()
{
#if defined(BOARD_ESP32_S3) || defined(BOARD_ESP32_C5)
    neoSet(0, NEO_BRIGHT / 2, NEO_BRIGHT / 2);
#else
    this->on();
#endif
}

// ── scanning ─────────────────────────────────────────────────────────────────
void LED::scanning()
{
#if defined(BOARD_ESP32_S3) || defined(BOARD_ESP32_C5)
    neoBlue();
#endif
}

// ── connecting ───────────────────────────────────────────────────────────────
void LED::connecting()
{
#if defined(BOARD_ESP32_S3) || defined(BOARD_ESP32_C5)
    neoYellow();
#endif
}

// ── connectedReady ───────────────────────────────────────────────────────────
// Green on for 2 s -> off.  Called once immediately after a successful join.
// Resets the heartbeat timer so the first pulse fires 30 s from now.
void LED::connectedReady()
{
#if defined(BOARD_ESP32_S3) || defined(BOARD_ESP32_C5)
    neoGreen();
    delay(2000);
    neoOff();
    _conn_last_pulse = millis();
    _conn_phase      = 0;
#endif
}

// ── connectedTick ────────────────────────────────────────────────────────────
// Non-blocking heartbeat.  Call every loop iteration while WiFi is connected.
// Every 30 s: fade green in over FADE_HALF_MS, then fade out over FADE_HALF_MS.
// LED is off between pulses.
void LED::connectedTick()
{
#if defined(BOARD_ESP32_S3) || defined(BOARD_ESP32_C5)
    uint32_t now     = millis();
    uint32_t elapsed = now - _conn_phase_start;

    if (_conn_phase == 0)
    {
        // Idle — wait for the 30 s interval
        if (now - _conn_last_pulse >= 30000)
        {
            _conn_last_pulse  = now;
            _conn_phase_start = now;
            _conn_phase       = 1;
        }
    }
    else if (_conn_phase == 1)
    {
        // Fade in: brightness ramps 0 → NEO_BRIGHT over FADE_HALF_MS
        if (elapsed >= FADE_HALF_MS)
        {
            neoGreen(); // ensure we hit full brightness
            _conn_phase_start = now;
            _conn_phase       = 2;
        }
        else
        {
            uint8_t br = (uint8_t)((uint32_t)NEO_BRIGHT * elapsed / FADE_HALF_MS);
            neoSet(0, br, 0);
        }
    }
    else // phase == 2
    {
        // Fade out: brightness ramps NEO_BRIGHT → 0 over FADE_HALF_MS
        if (elapsed >= FADE_HALF_MS)
        {
            neoOff();
            _conn_phase = 0;
        }
        else
        {
            uint8_t br = (uint8_t)((uint32_t)NEO_BRIGHT * (FADE_HALF_MS - elapsed) / FADE_HALF_MS);
            neoSet(0, br, 0);
        }
    }
#endif
}

// ── disconnected ─────────────────────────────────────────────────────────────
void LED::disconnected()
{
#if defined(BOARD_ESP32_S3) || defined(BOARD_ESP32_C5)
    neoRed();
    delay(400);
    neoOff();
#endif
}

// ── connectFailed ────────────────────────────────────────────────────────────
void LED::connectFailed()
{
#if defined(BOARD_ESP32_S3) || defined(BOARD_ESP32_C5)
    neoRed();
    delay(1000);
    neoOff();
#endif
}

// ── scanDone ─────────────────────────────────────────────────────────────────
void LED::scanDone(bool found)
{
#if defined(BOARD_ESP32_S3) || defined(BOARD_ESP32_C5)
    if (found)
    {
        neoYellow();
        delay(300);
    }
    else
    {
        neoRed();
        delay(600);
    }
    neoOff();
#else
    (void)found;
#endif
}
