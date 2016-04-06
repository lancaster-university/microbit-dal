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

/**
  * Definition of the LED Matrix maps supported.
  * Each map represents the layou of a different device.
  *
  * Ensure only one of these is selected.
  */

#ifndef MICROBIT_MATRIX_MAPS_H
#define MICROBIT_MATRIX_MAPS_H

#include "mbed.h"
#include "MicroBitDisplay.h"
/**
  * Provides the mapping from Matrix ROW/COL to a linear X/Y buffer. 
  * It's arranged such that matrixMap[col, row] provides the [x,y] screen co-ord.     
  */

#define NO_CONN 0

#if MICROBIT_DISPLAY_TYPE == MICROBUG_REFERENCE_DEVICE
    const MatrixPoint MicroBitDisplay::matrixMap[MICROBIT_DISPLAY_COLUMN_COUNT][MICROBIT_DISPLAY_ROW_COUNT] = 
    {   {{0,0},{0,1},{0,2},{0,3},{0,4}},
        {{1,0},{1,1},{1,2},{1,3},{1,4}},
        {{2,0},{2,1},{2,2},{2,3},{2,4}},
        {{3,0},{3,1},{3,2},{3,3},{3,4}},
        {{4,0},{4,1},{4,2},{4,3},{4,4}}
    };
#endif

#if MICROBIT_DISPLAY_TYPE == MICROBIT_3X9
    const MatrixPoint MicroBitDisplay::matrixMap[MICROBIT_DISPLAY_COLUMN_COUNT][MICROBIT_DISPLAY_ROW_COUNT] = 
    {   
        {{0,4},{0,3},{1,1}},
        {{1,4},{4,2},{0,1}},
        {{2,4},{3,2},{4,0}},
        {{3,4},{2,2},{3,0}},
        {{4,4},{1,2},{2,0}},
        {{4,3},{0,2},{1,0}},
        {{3,3},{4,1},{0,0}},
        {{2,3},{3,1},{NO_CONN,NO_CONN}},
        {{1,3},{2,1},{NO_CONN,NO_CONN}}
    };
#endif

#if MICROBIT_DISPLAY_TYPE == MICROBIT_SB1
    const MatrixPoint MicroBitDisplay::matrixMap[MICROBIT_DISPLAY_COLUMN_COUNT][MICROBIT_DISPLAY_ROW_COUNT] = 
    {   
        {{0,4},{1,4},{2,4},{3,4},{4,4},{4,3},{3,3},{2,3},{1,3}},
        {{0,3},{4,2},{3,2},{2,2},{1,2},{0,2},{4,1},{3,1},{2,1}},
        {{1,1},{0,1},{4,0},{3,0},{2,0},{1,0},{0,0},{NO_CONN,NO_CONN},{NO_CONN,NO_CONN}}
    };
#endif

#if MICROBIT_DISPLAY_TYPE == MICROBIT_SB2
    const MatrixPoint MicroBitDisplay::matrixMap[MICROBIT_DISPLAY_COLUMN_COUNT][MICROBIT_DISPLAY_ROW_COUNT] = 
    {   
        {{0,0},{4,2},{2,4}},
        {{2,0},{0,2},{4,4}},
        {{4,0},{2,2},{0,4}},
        {{4,3},{1,0},{0,1}},
        {{3,3},{3,0},{1,1}},
        {{2,3},{3,4},{2,1}},
        {{1,3},{1,4},{3,1}},
        {{0,3},{NO_CONN,NO_CONN},{4,1}},
        {{1,2},{NO_CONN,NO_CONN},{3,2}}
    };
    
#endif

const PinName rowPins[MICROBIT_DISPLAY_ROW_COUNT] = {MICROBIT_DISPLAY_ROW_PINS};
const uint8_t panicFace[MICROBIT_DISPLAY_COLUMN_COUNT] = {0x1B, 0x1B,0x0,0x0E,0x11};

#endif  

