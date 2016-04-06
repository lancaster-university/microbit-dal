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
#include "MicroBitConfig.h"

#define MICROBIT_FONT_WIDTH 5
#define MICROBIT_FONT_HEIGHT 5
#define MICROBIT_FONT_ASCII_START 32
#define MICROBIT_FONT_ASCII_END 126

/**
  * Class definition for a MicrobitFont
  * This class represents a font that can be used by the display to render text.
  *
  * A MicroBitFont is 5x5.
  * Each Row is represented by a byte in the array.
  *
  * Row Format:
  *            ================================================================
  *            | Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0 |
  *            ================================================================
  *            |  N/A  |  N/A  |  N/A  | Col 1 | Col 2 | Col 3 | Col 4 | Col 5 |
  *            |  0x80 |  0x40 |  0x20 | 0x10  | 0x08  | 0x04  | 0x02  | 0x01  |
  *
  * Example: { 0x08, 0x08, 0x08, 0x0, 0x08 }
  *
  * The above will produce an exclaimation mark on the second column in form the left.
  *
  * We could compress further, but the complexity of decode would likely outweigh the gains.
  */
class MicroBitFont
{
    public:

    static const unsigned char* defaultFont;
    static MicroBitFont systemFont;

    const unsigned char* characters;

    int asciiEnd;

    /**
      * Constructor.
      *
      * Sets the font represented by this font object.
      *
      * @param font A pointer to the beginning of the new font.
      *
      * @param asciiEnd the char value at which this font finishes.
      */
    MicroBitFont(const unsigned char* font, int asciiEnd = MICROBIT_FONT_ASCII_END);

    /**
      * Default Constructor.
      *
      * Configures the default font for the display to use.
      */
    MicroBitFont();

    /**
      * Modifies the current system font to the given instance of MicroBitFont.
      *
      * @param font the new font that will be used to render characters on the display.
      */
    static void setSystemFont(MicroBitFont font);

    /**
      * Retreives the font object used for rendering characters on the display.
      */
    static MicroBitFont getSystemFont();

};

#endif
