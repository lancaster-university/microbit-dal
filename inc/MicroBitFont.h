/*
The MIT License (MIT)

Copyright (c) 2016 British Broadcasting Corporation.
This software is provided by Lancaster University by arrangement with the BBC.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef MICROBIT_FONT_H
#define MICROBIT_FONT_H

#include "mbed.h"

#define MICROBIT_FONT_WIDTH 5
#define MICROBIT_FONT_HEIGHT 5
#define MICROBIT_FONT_ASCII_START 32
#define MICROBIT_FONT_ASCII_END 126

/**
  * Class definition for a MicrobitFont
  * It represents a font that can be used by the display to render text.
  */
class MicroBitFont
{
    public:
    
    static const unsigned char* defaultFont;
    
    const unsigned char* characters;
    
    int asciiEnd;
    
    /**
      * Constructor.
      * Sets the font represented by this font object.
      * @param font A pointer to the beginning of the new font.
      * @param asciiEnd the char value at which this font finishes.
      *
      * @note see main_font_test.cpp in the test folder for an example.
      */
    MicroBitFont(const unsigned char* font, int asciiEnd = MICROBIT_FONT_ASCII_END); 
    
    /**
      * Default Constructor.
      * Sets the characters to defaultFont characters and asciiEnd to MICROBIT_FONT_ASCII_END.
      */
    MicroBitFont();
};

#endif
