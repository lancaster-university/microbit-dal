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
