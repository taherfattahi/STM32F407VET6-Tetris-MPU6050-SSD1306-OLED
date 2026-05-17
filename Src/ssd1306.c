/*
 * ssd1306.c  –  SSD1306 128×64 OLED driver over I2C (STM32F4 HAL-free, register-level)
 *
 * Wiring (default):
 *   SDA → PB7   SCL → PB6   (I2C1 alternate function AF4)
 *   VCC → 3.3 V             GND → GND
 */

#include "ssd1306.h"
#include "stm32f4xx.h"

/* ─────────────────────────────────────────────────────────────────────────────
 *  Tiny 5×7 font (ASCII 32-127)
 * ───────────────────────────────────────────────────────────────────────────*/
static const uint8_t font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* ' ' 32 */
    {0x00,0x00,0x5F,0x00,0x00}, /* '!' 33 */
    {0x00,0x07,0x00,0x07,0x00}, /* '"' 34 */
    {0x14,0x7F,0x14,0x7F,0x14}, /* '#' 35 */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* '$' 36 */
    {0x23,0x13,0x08,0x64,0x62}, /* '%' 37 */
    {0x36,0x49,0x55,0x22,0x50}, /* '&' 38 */
    {0x00,0x05,0x03,0x00,0x00}, /* ''' 39 */
    {0x00,0x1C,0x22,0x41,0x00}, /* '(' 40 */
    {0x00,0x41,0x22,0x1C,0x00}, /* ')' 41 */
    {0x14,0x08,0x3E,0x08,0x14}, /* '*' 42 */
    {0x08,0x08,0x3E,0x08,0x08}, /* '+' 43 */
    {0x00,0x50,0x30,0x00,0x00}, /* ',' 44 */
    {0x08,0x08,0x08,0x08,0x08}, /* '-' 45 */
    {0x00,0x60,0x60,0x00,0x00}, /* '.' 46 */
    {0x20,0x10,0x08,0x04,0x02}, /* '/' 47 */
    {0x3E,0x51,0x49,0x45,0x3E}, /* '0' 48 */
    {0x00,0x42,0x7F,0x40,0x00}, /* '1' 49 */
    {0x42,0x61,0x51,0x49,0x46}, /* '2' 50 */
    {0x21,0x41,0x45,0x4B,0x31}, /* '3' 51 */
    {0x18,0x14,0x12,0x7F,0x10}, /* '4' 52 */
    {0x27,0x45,0x45,0x45,0x39}, /* '5' 53 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* '6' 54 */
    {0x01,0x71,0x09,0x05,0x03}, /* '7' 55 */
    {0x36,0x49,0x49,0x49,0x36}, /* '8' 56 */
    {0x06,0x49,0x49,0x29,0x1E}, /* '9' 57 */
    {0x00,0x36,0x36,0x00,0x00}, /* ':' 58 */
    {0x00,0x56,0x36,0x00,0x00}, /* ';' 59 */
    {0x08,0x14,0x22,0x41,0x00}, /* '<' 60 */
    {0x14,0x14,0x14,0x14,0x14}, /* '=' 61 */
    {0x00,0x41,0x22,0x14,0x08}, /* '>' 62 */
    {0x02,0x01,0x51,0x09,0x06}, /* '?' 63 */
    {0x32,0x49,0x79,0x41,0x3E}, /* '@' 64 */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 'A' 65 */
    {0x7F,0x49,0x49,0x49,0x36}, /* 'B' 66 */
    {0x3E,0x41,0x41,0x41,0x22}, /* 'C' 67 */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 'D' 68 */
    {0x7F,0x49,0x49,0x49,0x41}, /* 'E' 69 */
    {0x7F,0x09,0x09,0x09,0x01}, /* 'F' 70 */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 'G' 71 */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 'H' 72 */
    {0x00,0x41,0x7F,0x41,0x00}, /* 'I' 73 */
    {0x20,0x40,0x41,0x3F,0x01}, /* 'J' 74 */
    {0x7F,0x08,0x14,0x22,0x41}, /* 'K' 75 */
    {0x7F,0x40,0x40,0x40,0x40}, /* 'L' 76 */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* 'M' 77 */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 'N' 78 */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 'O' 79 */
    {0x7F,0x09,0x09,0x09,0x06}, /* 'P' 80 */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 'Q' 81 */
    {0x7F,0x09,0x19,0x29,0x46}, /* 'R' 82 */
    {0x46,0x49,0x49,0x49,0x31}, /* 'S' 83 */
    {0x01,0x01,0x7F,0x01,0x01}, /* 'T' 84 */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 'U' 85 */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 'V' 86 */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 'W' 87 */
    {0x63,0x14,0x08,0x14,0x63}, /* 'X' 88 */
    {0x07,0x08,0x70,0x08,0x07}, /* 'Y' 89 */
    {0x61,0x51,0x49,0x45,0x43}, /* 'Z' 90 */
    {0x00,0x7F,0x41,0x41,0x00}, /* '[' 91 */
    {0x02,0x04,0x08,0x10,0x20}, /* '\' 92 */
    {0x00,0x41,0x41,0x7F,0x00}, /* ']' 93 */
    {0x04,0x02,0x01,0x02,0x04}, /* '^' 94 */
    {0x40,0x40,0x40,0x40,0x40}, /* '_' 95 */
    {0x00,0x01,0x02,0x04,0x00}, /* '`' 96 */
    {0x20,0x54,0x54,0x54,0x78}, /* 'a' 97 */
    {0x7F,0x48,0x44,0x44,0x38}, /* 'b' 98 */
    {0x38,0x44,0x44,0x44,0x20}, /* 'c' 99 */
    {0x38,0x44,0x44,0x48,0x7F}, /* 'd' 100 */
    {0x38,0x54,0x54,0x54,0x18}, /* 'e' 101 */
    {0x08,0x7E,0x09,0x01,0x02}, /* 'f' 102 */
    {0x0C,0x52,0x52,0x52,0x3E}, /* 'g' 103 */
    {0x7F,0x08,0x04,0x04,0x78}, /* 'h' 104 */
    {0x00,0x44,0x7D,0x40,0x00}, /* 'i' 105 */
    {0x20,0x40,0x44,0x3D,0x00}, /* 'j' 106 */
    {0x7F,0x10,0x28,0x44,0x00}, /* 'k' 107 */
    {0x00,0x41,0x7F,0x40,0x00}, /* 'l' 108 */
    {0x7C,0x04,0x18,0x04,0x78}, /* 'm' 109 */
    {0x7C,0x08,0x04,0x04,0x78}, /* 'n' 110 */
    {0x38,0x44,0x44,0x44,0x38}, /* 'o' 111 */
    {0x7C,0x14,0x14,0x14,0x08}, /* 'p' 112 */
    {0x08,0x14,0x14,0x18,0x7C}, /* 'q' 113 */
    {0x7C,0x08,0x04,0x04,0x08}, /* 'r' 114 */
    {0x48,0x54,0x54,0x54,0x20}, /* 's' 115 */
    {0x04,0x3F,0x44,0x40,0x20}, /* 't' 116 */
    {0x3C,0x40,0x40,0x40,0x7C}, /* 'u' 117 */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 'v' 118 */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 'w' 119 */
    {0x44,0x28,0x10,0x28,0x44}, /* 'x' 120 */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 'y' 121 */
    {0x44,0x64,0x54,0x4C,0x44}, /* 'z' 122 */
    {0x00,0x08,0x36,0x41,0x00}, /* '{' 123 */
    {0x00,0x00,0x7F,0x00,0x00}, /* '|' 124 */
    {0x00,0x41,0x36,0x08,0x00}, /* '}' 125 */
    {0x10,0x08,0x08,0x10,0x08}, /* '~' 126 */
};

/* ─────────────────────────────────────────────────────────────────────────────
 *  Frame buffer  128×64 / 8 = 1024 bytes
 * ───────────────────────────────────────────────────────────────────────────*/
static uint8_t ssd1306_buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

/* ─────────────────────────────────────────────────────────────────────────────
 *  Low-level I2C helpers  (register-level, no HAL)
 * ───────────────────────────────────────────────────────────────────────────*/
static void I2C_Start(void)
{
    SSD1306_I2C->CR1 |= I2C_CR1_START;
    while (!(SSD1306_I2C->SR1 & I2C_SR1_SB));
}

static void I2C_Stop(void)
{
    SSD1306_I2C->CR1 |= I2C_CR1_STOP;
}

static void I2C_SendAddr(uint8_t addr, uint8_t rw)
{
    SSD1306_I2C->DR = (addr) | rw;           /* rw=0 → write, 1 → read */
    while (!(SSD1306_I2C->SR1 & I2C_SR1_ADDR));
    (void)SSD1306_I2C->SR2;                  /* clear ADDR */
}

static void I2C_SendByte(uint8_t data)
{
    while (!(SSD1306_I2C->SR1 & I2C_SR1_TXE));
    SSD1306_I2C->DR = data;
}

static void I2C_WaitBTF(void)
{
    while (!(SSD1306_I2C->SR1 & I2C_SR1_BTF));
}

/* Send one or more bytes prefixed by a control byte */
static void SSD1306_WriteCmd(uint8_t cmd)
{
    I2C_Start();
    I2C_SendAddr(SSD1306_I2C_ADDR, 0);
    I2C_SendByte(0x00);   /* Co=0, D/C#=0 → command */
    I2C_SendByte(cmd);
    I2C_WaitBTF();
    I2C_Stop();
}

static void SSD1306_WriteData(const uint8_t *buf, uint16_t len)
{
    I2C_Start();
    I2C_SendAddr(SSD1306_I2C_ADDR, 0);
    I2C_SendByte(0x40);   /* Co=0, D/C#=1 → data */
    for (uint16_t i = 0; i < len; i++) {
        I2C_SendByte(buf[i]);
    }
    I2C_WaitBTF();
    I2C_Stop();
}

/* ─────────────────────────────────────────────────────────────────────────────
 *  GPIO / I2C peripheral init  (PB6=SCL, PB7=SDA, I2C1, AF4, 400 kHz @ 42 MHz APB1)
 * ───────────────────────────────────────────────────────────────────────────*/
static void SSD1306_I2C_Init(void)
{
    /* 1. Clocks */
    RCC->AHB1ENR  |= RCC_AHB1ENR_GPIOBEN;
    RCC->APB1ENR  |= RCC_APB1ENR_I2C1EN;

    /* 2. PB6 & PB7: AF4, open-drain, high-speed, pull-up */
    GPIOB->MODER   &= ~((3u<<12)|(3u<<14));
    GPIOB->MODER   |=  ((2u<<12)|(2u<<14));   /* AF mode */
    GPIOB->OTYPER  |=  (1<<6)|(1<<7);         /* open-drain */
    GPIOB->OSPEEDR |=  (3u<<12)|(3u<<14);     /* very high speed */
    GPIOB->PUPDR   &= ~((3u<<12)|(3u<<14));
    GPIOB->PUPDR   |=  (1u<<12)|(1u<<14);     /* pull-up */
    GPIOB->AFR[0]  &= ~(0xFF<<24);
    GPIOB->AFR[0]  |=  (4u<<24)|(4u<<28);     /* AF4 */

    /* 3. Reset I2C1 */
    I2C1->CR1 |= I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    /* 4. Configure: APB1=42 MHz, fast mode 400 kHz */
    I2C1->CR2   = 42;              /* peripheral clock in MHz */
    I2C1->CCR   = I2C_CCR_FS | 35; /* fast mode, CCR = 42e6/(400e3*3) ≈ 35 */
    I2C1->TRISE = 13;              /* (300ns/23.8ns) + 1 ≈ 13 */
    I2C1->CR1  |= I2C_CR1_PE;     /* enable */
}

/* ─────────────────────────────────────────────────────────────────────────────
 *  Public API
 * ───────────────────────────────────────────────────────────────────────────*/
void SSD1306_Init(void)
{
    SSD1306_I2C_Init();

    /* Standard SSD1306 init sequence */
    SSD1306_WriteCmd(0xAE); /* display off */
    SSD1306_WriteCmd(0x20); SSD1306_WriteCmd(0x00); /* horizontal addressing */
    SSD1306_WriteCmd(0xB0); /* page start address 0 */
    SSD1306_WriteCmd(0xC8); /* COM scan direction: remapped */
    SSD1306_WriteCmd(0x00); /* low col offset */
    SSD1306_WriteCmd(0x10); /* high col offset */
    SSD1306_WriteCmd(0x40); /* start line 0 */
    SSD1306_WriteCmd(0x81); SSD1306_WriteCmd(0xFF); /* max contrast */
    SSD1306_WriteCmd(0xA1); /* segment remap */
    SSD1306_WriteCmd(0xA6); /* normal display (not inverted) */
    SSD1306_WriteCmd(0xA8); SSD1306_WriteCmd(0x3F); /* mux 64 */
    SSD1306_WriteCmd(0xA4); /* output follows RAM */
    SSD1306_WriteCmd(0xD3); SSD1306_WriteCmd(0x00); /* no display offset */
    SSD1306_WriteCmd(0xD5); SSD1306_WriteCmd(0xF0); /* clock divide ratio */
    SSD1306_WriteCmd(0xD9); SSD1306_WriteCmd(0x22); /* pre-charge */
    SSD1306_WriteCmd(0xDA); SSD1306_WriteCmd(0x12); /* COM pins */
    SSD1306_WriteCmd(0xDB); SSD1306_WriteCmd(0x20); /* vcom deselect */
    SSD1306_WriteCmd(0x8D); SSD1306_WriteCmd(0x14); /* charge pump on */
    SSD1306_WriteCmd(0xAF); /* display on */

    SSD1306_Clear();
    SSD1306_UpdateScreen();
}

void SSD1306_UpdateScreen(void)
{
    for (uint8_t page = 0; page < 8; page++) {
        SSD1306_WriteCmd(0xB0 + page);
        SSD1306_WriteCmd(0x00);
        SSD1306_WriteCmd(0x10);
        SSD1306_WriteData(&ssd1306_buffer[page * SSD1306_WIDTH], SSD1306_WIDTH);
    }
}

void SSD1306_Clear(void)
{
    memset(ssd1306_buffer, 0, sizeof(ssd1306_buffer));
}

void SSD1306_DrawPixel(int16_t x, int16_t y, uint8_t color)
{
    if (x < 0 || x >= SSD1306_WIDTH || y < 0 || y >= SSD1306_HEIGHT) return;
    if (color)
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] |=  (1 << (y & 7));
    else
        ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y & 7));
}

void SSD1306_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
    for (int16_t i = x; i < x + w; i++)
        for (int16_t j = y; j < y + h; j++)
            SSD1306_DrawPixel(i, j, color);
}

void SSD1306_DrawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
    for (int16_t i = x; i < x + w; i++) {
        SSD1306_DrawPixel(i, y,         color);
        SSD1306_DrawPixel(i, y + h - 1, color);
    }
    for (int16_t j = y + 1; j < y + h - 1; j++) {
        SSD1306_DrawPixel(x,         j, color);
        SSD1306_DrawPixel(x + w - 1, j, color);
    }
}

void SSD1306_DrawChar(int16_t x, int16_t y, char c, uint8_t color)
{
    if (c < 32 || c > 126) c = '?';
    const uint8_t *glyph = font5x7[c - 32];
    for (uint8_t col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (uint8_t row = 0; row < 7; row++) {
            if (bits & (1 << row))
                SSD1306_DrawPixel(x + col, y + row, color);
        }
    }
}

void SSD1306_DrawString(int16_t x, int16_t y, const char *str, uint8_t color)
{
    while (*str) {
        SSD1306_DrawChar(x, y, *str++, color);
        x += 6; /* 5 px glyph + 1 px gap */
    }
}
