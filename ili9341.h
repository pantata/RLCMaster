/*
 * ili9341.h
 *
 *  Created on: 26. 3. 2015
 *      Author: ludek
 */

#ifndef ILI9341_H_
#define ILI9341_H_

#include <avr/io.h>

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

#define RESET  PB2
#define CS     PB0
#define DC     PB1
#define LED    PD7

#define TFT_CS_OFF  PORTB |=  (1 << CS)
#define TFT_CS_ON   PORTB &= ~(1 << CS)

#define TFT_SET_DATA  PORTB |=  (1 << DC)
#define TFT_SET_CMD   PORTB &= ~(1 << DC)

#define TFT_RST_OFF PORTB |=  (1 << RESET)
#define TFT_RST_ON  PORTB &= ~(1 << RESET)

#define TFT_BL_ON  PORTD |=  (1 << LED)
#define TFT_BL_OFF PORTD &= ~(1 << LED)

#define PORTRAIT 0
#define LANDSCAPE 1

#define X (orient==LANDSCAPE?319L:239L)
#define Y (orient==LANDSCAPE?239L:319L)

#define LEFT 0
#define RIGHT 9999
#define CENTER 9998
#define BOTTOM 9997
#define TOP 0

#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID   0x04
#define ILI9341_RDDST   0x09

#define ILI9341_SLPIN   0x10
#define ILI9341_SLPOUT  0x11
#define ILI9341_PTLON   0x12
#define ILI9341_NORON   0x13

#define ILI9341_RDMODE  0x0A
#define ILI9341_RDMADCTL  0x0B
#define ILI9341_RDPIXFMT  0x0C
#define ILI9341_RDIMGFMT  0x0A
#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_INVOFF  0x20
#define ILI9341_INVON   0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON  0x29

#define ILI9341_CASET   0x2A
#define ILI9341_PASET   0x2B
#define ILI9341_RAMWR   0x2C
#define ILI9341_RAMRD   0x2E

#define ILI9341_PTLAR   0x30
#define ILI9341_MADCTL  0x36
#define ILI9341_PIXFMT  0x3A

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR  0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1  0xC0
#define ILI9341_PWCTR2  0xC1
#define ILI9341_PWCTR3  0xC2
#define ILI9341_PWCTR4  0xC3
#define ILI9341_PWCTR5  0xC4
#define ILI9341_VMCTR1  0xC5
#define ILI9341_VMCTR2  0xC7

#define ILI9341_RDID1   0xDA
#define ILI9341_RDID2   0xDB
#define ILI9341_RDID3   0xDC
#define ILI9341_RDID4   0xDD

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1

// VGA color palette
#define VGA_BLACK		0x0000
#define VGA_WHITE		0xFFFF
#define VGA_RED			0xF800
#define VGA_GREEN		0x0400
#define VGA_BLUE		0x001F
#define VGA_SILVER		0xC618
#define VGA_GRAY		0x8410
#define VGA_MAROON		0x8000
#define VGA_YELLOW		0xFFE0
#define VGA_OLIVE		0x8400
#define VGA_LIME		0x07E0
#define VGA_AQUA		0x07FF
#define VGA_TEAL		0x0410
#define VGA_NAVY		0x0010
#define VGA_FUCHSIA		0xF81F
#define VGA_PURPLE		0x8010


#define ILI9341_BLACK       0x0000      /*   0,   0,   0 */
#define ILI9341_NAVY        0x000F      /*   0,   0, 128 */
#define ILI9341_DARKGREEN   0x03E0      /*   0, 128,   0 */
#define ILI9341_DARKCYAN    0x03EF      /*   0, 128, 128 */
#define ILI9341_MAROON      0x7800      /* 128,   0,   0 */
#define ILI9341_PURPLE      0x780F      /* 128,   0, 128 */
#define ILI9341_OLIVE       0x7BE0      /* 128, 128,   0 */
#define ILI9341_LIGHTGREY   0xC618      /* 192, 192, 192 */
#define ILI9341_DARKGREY    0x2945      /* 128, 128, 128 */
#define ILI9341_BLUE        0x001F      /*   0,   0, 255 */
#define ILI9341_GREEN       0x07E0      /*   0, 255,   0 */
#define ILI9341_CYAN        0x07FF      /*   0, 255, 255 */
#define ILI9341_RED         0xF800      /* 255,   0,   0 */
#define ILI9341_MAGENTA     0xF81F      /* 255,   0, 255 */
#define ILI9341_YELLOW      0xFFE0      /* 255, 255,   0 */
#define ILI9341_WHITE       0xFFFF      /* 255, 255, 255 */
#define ILI9341_ORANGE      0xFD20      /* 255, 165,   0 */
#define ILI9341_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
#define ILI9341_PINK        0xF81F

const uint16_t ili9341_colors[] = {ILI9341_BLACK,
		ILI9341_NAVY,
		ILI9341_DARKGREEN,
		ILI9341_DARKCYAN,
		ILI9341_MAROON,
		ILI9341_PURPLE,
		ILI9341_OLIVE,
		ILI9341_LIGHTGREY,
		ILI9341_DARKGREY,
		ILI9341_BLUE,
		ILI9341_GREEN,
		ILI9341_CYAN,
		ILI9341_RED,
		ILI9341_MAGENTA,
		ILI9341_YELLOW,
		ILI9341_WHITE,
		ILI9341_ORANGE,
		ILI9341_GREENYELLOW,
		ILI9341_PINK
};

struct _current_font
{
    const uint8_t* font;
    uint8_t x_size;
    uint8_t y_size;
    uint8_t offset;
    uint8_t numchars;
};

extern uint8_t fch, fcl, bch, bcl;
extern uint8_t orient;
extern _current_font cfont;

void tft_on();
void tft_off();
void tft_init();
void tft_init2();
void tft_setRotation(uint8_t m);
void tft_setColor(uint16_t  color);
void tft_setBackColor(uint16_t color);
void tft_setFont(const uint8_t* font);
uint8_t tft_getBackColor();
uint8_t tft_getColor();
void tft_setXY(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void tft_clrXY();
void tft_clrScr();
void tft_drawPixel(int16_t x, int16_t y);
void tft_printChar(uint8_t c, int16_t x, int16_t y);
void tft_print(const char *st, int16_t x, int16_t y) ;
void tft_drawHLine(int16_t x, int16_t y, int16_t l);
void tft_drawVLine(int16_t x, int16_t y, int16_t l);
void tft_drawRect(int16_t x1, int16_t y1,int16_t x2, int16_t y2);
void tft_fillRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
//void tft_fillRect2(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void tft_drawCircle(int16_t x, int16_t y, int16_t radius);
void tft_fillCircle(int16_t x, int16_t y, int16_t radius);
void tft_drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void tft_fillScr(uint16_t color);
void tft_setColor(uint8_t r, uint8_t g, uint8_t b);
uint8_t tft_getFontYsize();
uint8_t tft_getFontXsize();
void tft_printNumI(long num, int x, int y, int length=0, char filler=' ');

#endif /* ILI9341_H_ */
