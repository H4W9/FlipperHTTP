#pragma once
#include <Arduino.h>

#if defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
#include <TFT_eSPI.h>
#include <Arduino.h>
#elif defined(BOARD_CARDPUTER)
#include "cardputer/lcd.h"
#endif

class LCD
{
public:
    LCD();
    ~LCD();

    void clear(uint16_t color = 0x0000);
    void print(uint16_t x, uint16_t y, const char *text);
    void swap();
    void text(uint16_t x, uint16_t y, const char *text);

private:
#if defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    TFT_eSPI *displayTFT = nullptr;   //  TFT SPI library
    TFT_eSprite *canvasTFT = nullptr; // Off-screen buffer ("sprite")
#endif
};