/*
  GLCD84x48.cpp - Library for Graphic LCD 84x48.
  Created by Benjamin B., June 2, 2014.
  Released into the public domain.
*/

#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#include "GLCD84x48.h"

GLCD84x48::GLCD84x48(){}

void GLCD84x48::lcdWrite(byte d, byte data)
{
  digitalWrite(PIN_DC, d);
  digitalWrite(PIN_SCE, LOW);
  shiftOut(PIN_SDIN, PIN_SCLK, MSBFIRST, data);
  digitalWrite(PIN_SCE, HIGH);
}

byte GLCD84x48::validPosition(byte x, byte y)
{
	if (x < LCD_X && y < LCD_Y)
		return 1;
	
	return 0;
}

byte GLCD84x48::setPositionXY(byte x, byte y)
{
	if(!validPosition(x, y))
		return 0;
	
	lcdWrite(LOW, 0x80 | x);
	lcdWrite(LOW, 0x40 | y/8);
	return 1;
}

void GLCD84x48::clearRaster()
{
	for(byte i = 0; i < LCD_Y/8; i++)
		for(byte j = 0; j < LCD_X; j++)
			raster[i][j] = 0x00; 
}

void GLCD84x48::clearLcd()
{
	for (int i = 0; i < LCD_X * LCD_Y / 8; i++)
		lcdWrite(HIGH, 0x00);
	
	clearRaster();
}

void GLCD84x48::lcdInitialise(byte SCE, byte RESET, byte DC, byte SDIN, byte SCLK)
{
  PIN_SCE = SCE; PIN_RESET = RESET; PIN_DC = DC; PIN_SDIN = SDIN; PIN_SCLK = SCLK;
  pinMode(PIN_SCE,   OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_DC,    OUTPUT);
  pinMode(PIN_SDIN,  OUTPUT);
  pinMode(PIN_SCLK,  OUTPUT);
  digitalWrite(PIN_RESET, LOW);
  digitalWrite(PIN_RESET, HIGH);
  
  // LCD CONFIGURATION FOR 3.3V supply and logic
  lcdWrite( LOW, 0x21 );  // for extended instruction set
  lcdWrite( LOW, 0xC8 );  // operating voltage (Vop) of the LCD
  lcdWrite( LOW, 0x06 );  // temperature coefficient
  lcdWrite( LOW, 0x13 );  // bias system of the LCD
  lcdWrite( LOW, 0x20 );  // allow the LCD to operate in function set command with basic instruction
  lcdWrite( LOW, 0x0C );  // 0x0C in normal mode, 0x0d for inverse
  clearLcd();
}

// override = 1 if you want to override current state
void GLCD84x48::printCharAtXY(byte x, byte y, char character, byte override)
{
	if(!setPositionXY(x, y))
		return;
	
	for (byte i = 0; i < CHAR_WIDTH; i++)
	{
		raster[y/8][x+i] = (override)? ASCII[character - 0x20][i] : raster[y/8][x+i] | ASCII[character - 0x20][i];
		lcdWrite (HIGH, raster[y/8][x+i]);
	}
}

// x, y - coordinates of the upper left corner
// a, b - width and height
// set - 1 set, 0 clear pixel
// filled rectangle (line, square, pixel, rectangle)
void GLCD84x48::fRect(byte x, byte y, byte a, byte b, byte set)
{
	if(!validPosition(x, y) || !validPosition(x+a-1, y+b-1))
		return;
	
	byte e, row = y, offset;
	
	while(row < y+b)
	{
		lcdWrite(LOW, 0x80 | x);
		lcdWrite(LOW, 0x40 | row/8);
		
		for(byte column = x; column < x+a; column++)
		{
			e = POW_OF_TWO[row%8];
			
			for(byte offset = row+1; ((offset < y+b) && (offset%8 != 0)); offset++)
				e += POW_OF_TWO[offset%8];
			
			raster[row/8][column] = (set)? raster[row/8][column] | e : raster[row/8][column] & (~e);
            lcdWrite (HIGH, raster[row/8][column]);
		}
		row += 8-row%8;
	}	
}
// empty rectangle with thickness t
void GLCD84x48::eRect(byte x, byte y, byte a, byte b, byte set, byte t)
{
	if(!validPosition(x, y) || !validPosition(x+a-1, y+b-1) || t == 0)
		return;
	
	byte min = (a<b)? a : b;
	if(min/2 < t)
	{
		fRect(x, y, a, b, set);
		return;
	}
	
	fRect(x, y, t, b, set);
	fRect(x+t, y+b-t, a-2*t, t, set);
	fRect(x+a-t, y, t, b, set);
	fRect(x+t, y, a-2*t, t, set);
}

void GLCD84x48::setPixel(byte x, byte y, byte s)
{
	if(!setPositionXY(x, y))
		return;
	
	raster[y/8][x] = (s)? raster[y/8][x] | _BV(y%8) : raster[y/8][x] & (~_BV(y%8));
    lcdWrite (HIGH, raster[y/8][x]);
}

void GLCD84x48::cp(int dx, int dy, int x, int y, int s)
{
	if(x < y)
	{
		setPixel(dx + x, dy + y, s);
		setPixel(dx + x, dy - y, s);
		setPixel(dx + y, dy + x, s);
		setPixel(dx + y, dy - x, s);
        setPixel(dx - x, dy + y, s);        
        setPixel(dx - x, dy - y, s);        
        setPixel(dx - y, dy + x, s);        
        setPixel(dx - y, dy - x, s);
	}
	else if(x == y)
	{
		setPixel(dx + x, dy + y, s);
		setPixel(dx + x, dy - y, s);
        setPixel(dx - x, dy + y, s);        
        setPixel(dx - x, dy - y, s);
	}
	else if(x == 0)
	{
		setPixel(dx, dy + y, s);
        setPixel(dx, dy - y, s);
        setPixel(dx + y, dy, s);
        setPixel(dx - y, dy, s);
	}
}

void GLCD84x48::circle(byte x0, byte y0, byte radius, byte s)
{
    int x = 0, y = radius, p = (5 - radius*4)/4;

    cp(x0, y0, x, y, s);
    while (x < y) {		
        x++;		
        if (p < 0)
			p += 2*x+1;
		else
			p += 2*(x-(--y))+1;
		
        cp(x0, y0, x, y, s);
    }
}

void GLCD84x48::circle2(byte x0, byte y0, byte r, byte s)
{
	byte x = r, y = 0;
	int e = 0;

	while (x >= y)
	{
		setPixel(x0 + x, y0 + y, s);
		setPixel(x0 + x, y0 - y, s);
		setPixel(x0 + y, y0 + x, s);
		setPixel(x0 + y, y0 - x, s);		
		setPixel(x0 - y, y0 + x, s);
		setPixel(x0 - y, y0 - x, s);		
		setPixel(x0 - x, y0 + y, s);
		setPixel(x0 - x, y0 - y, s);	

		if(e > 0)
			e -= 2*(--x) + 1;
		else
			e += 2*(++y) + 1;
	}
}

void GLCD84x48::line(byte x0, byte y0, byte x1, byte y1, byte s)
{ 
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1; 
  int e1 = (dx > dy ? dx : - dy) / 2, e2;
  
  while(1)
  {
	  setPixel(x0, y0, s);
	  if(x0 == x1 && y0 == y1)
		  break;
	  
	  e2 = e1;
	  if(e2 < dy)
	  {
		  e1 += dx;
		  y0 += sy;
	  }
	  if(e2 > -dx)
	  {
		  e1 -= dy;
		  x0 += sx;
	  }
  }
}