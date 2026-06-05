#include "lcd.hpp"

#ifdef TFT_BLACK
#undef TFT_BLACK
#endif
#define TFT_BLACK 0x0000

#ifdef TFT_WHITE
#undef TFT_WHITE
#endif
#define TFT_WHITE 0xFFFF

LCD::LCD()
{
#if defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    this->displayTFT = new TFT_eSPI();
    this->displayTFT->init();
    delay(10); // Small delay after initialization
    this->displayTFT->setRotation(0);
    this->displayTFT->fillScreen(TFT_BLACK);

    this->canvasTFT = new TFT_eSprite(this->displayTFT);
    this->canvasTFT->setColorDepth(8);
    if (this->canvasTFT->createSprite(320, 320))
    {
        this->canvasTFT->setTextFont(2);
        this->canvasTFT->setTextSize(1);
        this->canvasTFT->setTextColor(TFT_WHITE);
        this->canvasTFT->fillSprite(TFT_BLACK);
    }
#elif defined(BOARD_CARDPUTER)
    lcd_init();
#endif
}

LCD::~LCD()
{
#if defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    if (this->canvasTFT)
    {
        this->canvasTFT->deleteSprite();
        delete this->canvasTFT;
    }
    if (this->displayTFT)
    {
        delete this->displayTFT;
    }
#elif defined(BOARD_CARDPUTER)
    lcd_deinit();
#endif
}

void LCD::clear(uint16_t color)
{
#if defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    if (this->canvasTFT)
    {
        this->canvasTFT->fillRect(0, 0, 320, 320, color);
    }
#elif defined(BOARD_CARDPUTER)
    lcd_fill(color);
#endif
}

void LCD::print(uint16_t x, uint16_t y, const char *text)
{
#if defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    if (this->canvasTFT)
    {
        this->canvasTFT->setCursor(x, y);
        this->canvasTFT->print(text);
    }
#elif defined(BOARD_CARDPUTER)
    lcd_draw_text(x, y, text, TFT_WHITE, FONT_DEFAULT);
#endif
}

void LCD::swap()
{
#if defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    if (this->canvasTFT)
    {
        this->canvasTFT->pushSprite(0, 0);
    }
#elif defined(BOARD_CARDPUTER)
    lcd_swap();
#endif
}

void LCD::text(uint16_t x, uint16_t y, const char *text)
{
#if defined(BOARD_PICOCALC_W) || defined(BOARD_PICOCALC_2W)
    if (this->canvasTFT && this->displayTFT)
    {
        this->clear(TFT_BLACK);
        this->print(x, y, text);
        this->swap();
    }
#elif defined(BOARD_CARDPUTER)
    this->clear(TFT_BLACK);
    this->print(x, y, text);
    this->swap();
#endif
}
