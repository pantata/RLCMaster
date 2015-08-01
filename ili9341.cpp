/*
 * ili9341.c
 *
 *  Created on: 26. 3. 2015
 *      Author: ludek
 */

#include <stdint.h>
#include <string.h>
#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>
#include <avr/pgmspace.h>


#include "ili9341.h"
#include "spi.h"

#define fontbyte(x) pgm_read_byte(&cfont.font[x])
#define false 0
#define true  1

uint8_t fch, fcl, bch, bcl;
uint8_t orient;
struct _current_font	cfont;

void tft_on() {
    TFT_CS_ON;
    TFT_SET_CMD;
    spi_rw(0x29);
    TFT_CS_OFF;
    TFT_BL_ON;
}

void tft_off() {
    TFT_CS_ON;
    TFT_SET_CMD;
    spi_rw(0x28);
    TFT_CS_OFF;
    TFT_BL_OFF;
}



void tft_init() {

	//CS, DC, LED na vystup
	DDRB |= (1 << CS) | (1 << DC);
	DDRD |= (1 << LED);

    //backlight on
	 TFT_BL_ON;

    _delay_ms(20);

    //select LCD
    TFT_CS_ON;
    TFT_SET_DATA;
    //reset
    TFT_RST_ON;
    _delay_ms(20);
    TFT_RST_OFF;
    _delay_ms(150);

    //power control A
    TFT_SET_CMD;
    spi_rw(0xCB);
    TFT_SET_DATA;
    spi_rw(0x39);
    spi_rw(0x2C);
    spi_rw(0x00);
    spi_rw(0x34);
    spi_rw(0x02);

    //power control B
    TFT_SET_CMD;
    spi_rw(0xCF);
    TFT_SET_DATA;
    spi_rw(0x00);
    spi_rw(0XC1);
    spi_rw(0X30);


    //Driver timing control A
    TFT_SET_CMD;
    spi_rw(0xE8);
    TFT_SET_DATA;
    spi_rw(0x85);
    spi_rw(0x00);
    spi_rw(0x78);

    //Driver timing control B
    TFT_SET_CMD;
    spi_rw(0xEA);
    TFT_SET_DATA;
    spi_rw(0x00);
    spi_rw(0x00);

    //Power on sequence control
    TFT_SET_CMD;
    spi_rw(0xED);
    TFT_SET_DATA;
    spi_rw(0x64);
    spi_rw(0x03);
    spi_rw(0X12);
    spi_rw(0X81);

    //Pump ratio control
    TFT_SET_CMD;
    spi_rw(0xF7);
    TFT_SET_DATA;
    spi_rw(0x20);

    TFT_SET_CMD;
    spi_rw(0xC0);    	//Power control 1
    spi_rw(0x23);   	//VRH[5:0], 4.6V ?? 00001001 = 3.3V

    TFT_SET_CMD;
    spi_rw(0xC1);    	//Power control 2
    TFT_SET_DATA;
    spi_rw(0x10);   	//SAP[2:0];BT[3:0]

    TFT_SET_CMD;
    spi_rw(0xC5);    	//VCM control
    TFT_SET_DATA;
    spi_rw(0x3e);   	//Contrast
    spi_rw(0x28);

    TFT_SET_CMD;
    spi_rw(0xC7);    	//VCM control2
    TFT_SET_DATA;
    spi_rw(0x86);  	 //--

    TFT_SET_CMD;
    spi_rw(0x36);    	// Memory Access Control
    TFT_SET_DATA;
    spi_rw(0x48);  	//C8	   //48 68绔栧睆//28 E8 妯睆


    //pixel format set  16bit
    TFT_SET_CMD;
    spi_rw(0x3A);
    TFT_SET_DATA;
    spi_rw(0x55);

    //Frame Rate Control 79Hz
    TFT_SET_CMD;
    spi_rw(0xB1);
    TFT_SET_DATA;
    spi_rw(0x00);
    spi_rw(0x18);

    // Display Function Control
    TFT_SET_CMD;
    spi_rw(0xB6);
    TFT_SET_DATA;
    spi_rw(0x08);
    spi_rw(0x82);
    spi_rw(0x27);

    TFT_SET_CMD;
    spi_rw(0xF2);    	// 3Gamma Function Disable
    TFT_SET_DATA;
    spi_rw(0x00);

    TFT_SET_CMD;
    spi_rw(0x26);    	//Gamma curve selected
    TFT_SET_DATA;
    spi_rw(0x01);

    TFT_SET_CMD;
    spi_rw(0xE0);    	//Set Gamma
    TFT_SET_DATA;
    spi_rw(0x0F);
    spi_rw(0x31);
    spi_rw(0x2B);
    spi_rw(0x0C);
    spi_rw(0x0E);
    spi_rw(0x08);
    spi_rw(0x4E);
    spi_rw(0xF1);
    spi_rw(0x37);
    spi_rw(0x07);
    spi_rw(0x10);
    spi_rw(0x03);
    spi_rw(0x0E);
    spi_rw(0x09);
    spi_rw(0x00);

    TFT_SET_CMD;
    spi_rw(0XE1);    	//Set Gamma
    TFT_SET_DATA;
    spi_rw(0x00);
    spi_rw(0x0E);
    spi_rw(0x14);
    spi_rw(0x03);
    spi_rw(0x11);
    spi_rw(0x07);
    spi_rw(0x31);
    spi_rw(0xC1);
    spi_rw(0x48);
    spi_rw(0x08);
    spi_rw(0x0F);
    spi_rw(0x0C);
    spi_rw(0x31);
    spi_rw(0x36);
    spi_rw(0x0F);

    TFT_SET_CMD;
    spi_rw(0x38);  //iddle off

    spi_rw(0x11);    	//Exit Sleep
    _delay_ms(100);

    spi_rw(0x29);    //Display on
    spi_rw(0x2c);
    TFT_CS_OFF;

    cfont.font = 0;
    orient=PORTRAIT;
    tft_setColor(VGA_WHITE);
    tft_setBackColor(VGA_BLACK);

}

void tft_setRotation(uint8_t m) {

    uint8_t rotation = m % 4; // can't be higher than 3
    TFT_CS_ON;
    TFT_SET_CMD;
    spi_rw(0x36);
    TFT_SET_DATA;
    switch (rotation) {
        case 0:
            spi_rw(MADCTL_MX | MADCTL_BGR);
            orient=PORTRAIT;
            break;
        case 1:
            spi_rw(MADCTL_MV | MADCTL_BGR);
            orient=LANDSCAPE;
            break;
        case 2:
            spi_rw(MADCTL_MY | MADCTL_BGR);
            orient=PORTRAIT;
            break;
        case 3:
            spi_rw(MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
            orient=LANDSCAPE;
            break;
    }
    TFT_CS_OFF;
}

void tft_setFont(const uint8_t* font) {
    cfont.font=font;
    cfont.x_size=fontbyte(0);
    cfont.y_size=fontbyte(1);
    cfont.offset=fontbyte(2);
    cfont.numchars=fontbyte(3);
}

void tft_setColor(uint16_t  color) {
    fch=(color>>8);
    fcl=(color & 0xFF);
}

void tft_setBackColor(uint8_t r, uint8_t g, uint8_t b) {
	bch=((r&248)|g>>5);
	bcl=((g&28)<<3|b>>3);
}

void tft_setBackColor(uint16_t color) {

    bch=(color>>8);
    bcl=(color & 0xFF);
}

uint8_t tft_getBackColor() {
    return (bch<<8) | bcl;
}
uint8_t tft_getColor() {
    return (fch<<8) | fcl;
}

uint8_t tft_getFontXsize() {
	return cfont.x_size;
}


uint8_t tft_getFontYsize() {
	return cfont.y_size;
}

void tft_setXY(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {

    TFT_SET_CMD;
    spi_rw(0x2a);
    TFT_SET_DATA;
    spi_rw(x1>>8);
    spi_rw(x1);
    spi_rw(x2>>8);
    spi_rw(x2);
    TFT_SET_CMD;
    spi_rw(0x2b);
    TFT_SET_DATA;
    spi_rw(y1>>8);
    spi_rw(y1);
    spi_rw(y2>>8);
    spi_rw(y2);
    TFT_SET_CMD;
    spi_rw(0x2c);
    TFT_SET_DATA;
}

void tft_clrXY() {
    tft_setXY(0,0,X,Y);
}

void tft_drawPixel(int16_t x, int16_t y) {
    TFT_CS_ON;
    tft_setXY(x, y, x, y);
    spi_rw(fch); spi_rw(fcl);
    tft_clrXY();
    TFT_CS_OFF;
}

void tft_clrScr() {
    TFT_CS_ON;
    tft_setXY(0,0,X,Y);
    for (uint32_t i=0; i<76800; i++) {
        spi_rw(bch);
        spi_rw(bcl);
    }
    tft_clrXY();
    TFT_CS_OFF;
}

void tft_setColor(uint8_t r, uint8_t g, uint8_t b)
{
	fch=((r&248)|g>>5);
	fcl=((g&28)<<3|b>>3);
}


void tft_printChar(uint8_t c, int16_t x, int16_t y) {
    uint8_t i,ch;
    int16_t j;
    uint16_t pos;
    TFT_CS_ON;
    tft_setXY(x,y,x+cfont.x_size-1,y+cfont.y_size-1); //nastavim oblast na zapis

    pos=((c-cfont.offset)*((cfont.x_size/8)*cfont.y_size))+4; //spocitame start pozici znaku v poli

    for(j=0;j<((cfont.x_size/8)*cfont.y_size);j++) {
        ch=pgm_read_byte(&cfont.font[pos]);
        for(i=0;i<8;i++) {  //zapisujeme do pameti
            if((ch&(1<<(7-i)))!=0) {
                spi_rw(fch);
                spi_rw(fcl);
            } else {
                spi_rw(bch);
                spi_rw(bcl);
            }
        }
        pos++;
    }
    tft_clrXY();
    TFT_CS_OFF;
}

void tft_print(const char *st, int16_t x, int16_t y) {
    int stl, i;
    stl = strlen(st);

    if (x == RIGHT) x=(X+1)-(stl*cfont.x_size);
    if (x == CENTER) x=((X+1)-(stl*cfont.x_size))/2;

    if (y == BOTTOM) y=(Y+1)-cfont.y_size;
    for (i=0; i<stl; i++)
        tft_printChar(*st++, x + (i*(cfont.x_size)), y);
}


void tft_printNumI(long num, int x, int y, int length, char filler)
{
	char buf[25];
	char st[27];
	uint8_t neg=false;
	int c=0, f=0;

	if (num==0)
	{
		if (length!=0)
		{
			for (c=0; c<(length-1); c++)
				st[c]=filler;
			st[c]=48;
			st[c+1]=0;
		}
		else
		{
			st[0]=48;
			st[1]=0;
		}
	}
	else
	{
		if (num<0)
		{
			neg=true;
			num=-num;
		}

		while (num>0)
		{
			buf[c]=48+(num % 10);
			c++;
			num=(num-(num % 10))/10;
		}
		buf[c]=0;

		if (neg)
		{
			st[0]=45;
		}

		if (length>(c+neg))
		{
			for (int i=0; i<(length-c-neg); i++)
			{
				st[i+neg]=filler;
				f++;
			}
		}

		for (int i=0; i<c; i++)
		{
			st[i+neg+f]=buf[c-i-1];
		}
		st[c+neg+f]=0;

	}

	tft_print(st,x,y);
}

void tft_drawHLine(int16_t x, int16_t y, int16_t l) {
    if (l<0)
    {
        l = -l;
        x -= l;
    }
    TFT_CS_ON;
    tft_setXY(x, y, x+l, y);
    for (int16_t i=0; i<l+1; i++)
    {
        spi_rw(fch);
        spi_rw(fcl);
    }
    tft_clrXY();
    TFT_CS_OFF;
}

void tft_drawVLine(int16_t x, int16_t y, int16_t l) {
    if (l<0)
    {
        l = -l;
        y -= l;
    }
    TFT_CS_ON;
    tft_setXY(x, y, x, y+l);
    for (int16_t i=0; i<l+1; i++)
    {
        spi_rw(fch);
        spi_rw(fcl);
    }
    tft_clrXY();
    TFT_CS_OFF;
}

void tft_drawRect(int16_t x1, int16_t y1,int16_t x2, int16_t y2)
{
    tft_drawHLine(x1, y1, x2-x1);
    tft_drawHLine(x1, y2, x2-x1);
    tft_drawVLine(x1, y1, y2-y1);
    tft_drawVLine(x2, y1, y2-y1);
}

void tft_fillRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {

	int16_t tmp;

    if (x1>x2)
    {
        tmp = x1; x1=x2;x2=tmp;
    }

    if (y1>y2)
    {
        tmp = y1; y1=y2;y2=tmp;
    }

	TFT_CS_ON;
	tft_setXY(x1, y1, x2, y2);
    for (uint32_t i=0; i< ((uint32_t)(x2-x1+1)) * ((uint32_t)(y2-y1+1))   ; i++)
    {
        spi_rw(fch);
        spi_rw(fcl);
    }
    tft_clrXY();
    TFT_CS_OFF;
}

/*
void tft_fillRect(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
	int16_t tmp;

    if (x1>x2)
    {
        tmp = x1; x1=x2;x2=tmp;
    }

    if (y1>y2)
    {
        tmp = y1; y1=y2;y2=tmp;
    }

    for (int i=0; i<((x2-x1)/2)+1; i++)
     {
    	tft_drawVLine(x1+i, y1, y2-y1);
    	tft_drawVLine(x2-i, y1, y2-y1);
     }

}
*/
void tft_drawCircle(int16_t x, int16_t y, int16_t radius)
{
    int16_t f = 1 - radius;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * radius;
    int16_t x1 = 0;
    int16_t y1 = radius;

    TFT_CS_ON;
    tft_setXY(x, y + radius, x, y + radius);
    spi_rw(fch);spi_rw(fcl);
    tft_setXY(x, y - radius, x, y - radius);
    spi_rw(fch);spi_rw(fcl);
    tft_setXY(x + radius, y, x + radius, y);
    spi_rw(fch);spi_rw(fcl);
    tft_setXY(x - radius, y, x - radius, y);
    spi_rw(fch);spi_rw(fcl);

    while(x1 < y1)
    {
        if(f >= 0)
        {
            y1--;
            ddF_y += 2;
            f += ddF_y;
        }
        x1++;
        ddF_x += 2;
        f += ddF_x;
        tft_setXY(x + x1, y + y1, x + x1, y + y1);
        spi_rw(fch);spi_rw(fcl);
        tft_setXY(x - x1, y + y1, x - x1, y + y1);
        spi_rw(fch);spi_rw(fcl);
        tft_setXY(x + x1, y - y1, x + x1, y - y1);
        spi_rw(fch);spi_rw(fcl);
        tft_setXY(x - x1, y - y1, x - x1, y - y1);
        spi_rw(fch);spi_rw(fcl);
        tft_setXY(x + y1, y + x1, x + y1, y + x1);
        spi_rw(fch);spi_rw(fcl);
        tft_setXY(x - y1, y + x1, x - y1, y + x1);
        spi_rw(fch);spi_rw(fcl);
        tft_setXY(x + y1, y - x1, x + y1, y - x1);
        spi_rw(fch);spi_rw(fcl);
        tft_setXY(x - y1, y - x1, x - y1, y - x1);
        spi_rw(fch);spi_rw(fcl);
    }
    tft_clrXY();
    TFT_CS_OFF;
}

void tft_fillCircle(int16_t x, int16_t y, int16_t radius)
{
    for(int16_t y1=-radius; y1<=0; y1++)
        for(int16_t x1=-radius; x1<=0; x1++)
            if(x1*x1+y1*y1 <= radius*radius)
            {
            	tft_drawHLine(x+x1, y+y1, 2*(-x1));
            	tft_drawHLine(x+x1, y-y1, 2*(-x1));
                break;
            }

}

void tft_drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2)
           {
               if (y1==y2)
            	   tft_drawHLine(x1, y1, x2-x1);
               else if (x1==x2)
            	   tft_drawVLine(x1, y1, y2-y1);
               else
               {
                   int16_t	dx = (x2 > x1 ? x2 - x1 : x1 - x2);
                   int8_t		xstep =  x2 > x1 ? 1 : -1;
                   int16_t	dy = (y2 > y1 ? y2 - y1 : y1 - y2);
                   int8_t		ystep =  y2 > y1 ? 1 : -1;
                   int16_t	col = x1, row = y1;
                   TFT_CS_ON;
                   if (dx < dy)
                   {
                       int16_t t = - (dy >> 1);
                       while (1)
                       {
                    	   tft_setXY (col, row, col, row);
                           spi_rw(fch);spi_rw(fcl);
                           if (row == y2)
                               break;
                           row += ystep;
                           t += dx;
                           if (t >= 0)
                           {
                               col += xstep;
                               t   -= dy;
                           }
                       }
                   }
                   else
                   {
                       int16_t t = - (dx >> 1);
                       while (1)
                       {
                    	   tft_setXY (col, row, col, row);
                           spi_rw(fch);spi_rw(fcl);
                           if (col == x2)
                               break;
                           col += xstep;
                           t += dy;
                           if (t >= 0)
                           {
                               row += ystep;
                               t   -= dx;
                           }
                       }
                   }
                   tft_clrXY();
                   TFT_CS_OFF;
               }
           }

void tft_fillScr(uint16_t color) {
	tft_setBackColor(color);
    tft_clrScr();
}

