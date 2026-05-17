#ifndef SSD1306_H
#define SSD1306_H

#include "stm32f4xx.h"
#include <stdint.h>
#include <string.h>

/* ── Display dimensions ── */
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT   64

/* ── I2C address (SA0 pin low → 0x3C, high → 0x3D) ── */
#define SSD1306_I2C_ADDR   (0x3C << 1)

/* ── I2C peripheral (change to I2C2/I2C3 if needed) ── */
#define SSD1306_I2C        I2C1
#define SSD1306_I2C_TIMEOUT 10000

/* ── Pixel colours ── */
#define SSD1306_COLOR_BLACK  0
#define SSD1306_COLOR_WHITE  1

/* ── Public API ── */
void SSD1306_Init(void);
void SSD1306_UpdateScreen(void);
void SSD1306_Clear(void);
void SSD1306_DrawPixel(int16_t x, int16_t y, uint8_t color);
void SSD1306_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void SSD1306_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void SSD1306_DrawChar(int16_t x, int16_t y, char c, uint8_t color);
void SSD1306_DrawString(int16_t x, int16_t y, const char *str, uint8_t color);

#endif /* SSD1306_H */
