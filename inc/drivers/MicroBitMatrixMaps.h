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

#include "MicroBitConfig.h"
#include "mbed.h"

#define NO_CONN 0

/**
  * Provides the mapping from Matrix ROW/COL to a linear X/Y buffer.
  * It's arranged such that matrixMap[col, row] provides the [x,y] screen co-ord.
  */

struct MatrixPoint
{
    uint8_t x;
    uint8_t y;
};

/**
  * This struct presumes rows and columns are arranged contiguously...
  */
struct MatrixMap
{
    int         width;                      // The physical width of the LED matrix, in pixels.
    int         height;                     // The physical height of the LED matrix, in pixels.
    int         rows;                       // The number of drive pins connected to LEDs.
    int         columns;                    // The number of sink pins connected to the LEDs.

    PinName     rowStart;                   // ID of the first drive pin.
    PinName     columnStart;                // ID of the first sink pink.

    const MatrixPoint *map;                 // Table mapping logical LED positions to physical positions.
};

/*
 * Dimensions for well known micro:bit LED configurations
 */
#define MICROBIT_DISPLAY_WIDTH                  5
#define MICROBIT_DISPLAY_HEIGHT                 5
#define MICROBIT_DISPLAY_ROW1                   p13
#define MICROBIT_DISPLAY_COL1                   p4


#if MICROBIT_DISPLAY_TYPE == MICROBUG_REFERENCE_DEVICE

#define MICROBIT_DISPLAY_COLUMN_COUNT           5
#define MICROBIT_DISPLAY_ROW_COUNT              5

    const MatrixPoint microbitDisplayMap[MICROBIT_DISPLAY_ROW_COUNT * MICROBIT_DISPLAY_COLUMN_COUNT] =
    {
        {0,0},{0,1},{0,2},{0,3},{0,4},
        {1,0},{1,1},{1,2},{1,3},{1,4},
        {2,0},{2,1},{2,2},{2,3},{2,4},
        {3,0},{3,1},{3,2},{3,3},{3,4},
        {4,0},{4,1},{4,2},{4,3},{4,4}
    };

#endif

#if MICROBIT_DISPLAY_TYPE == MICROBIT_3X9

#define MICROBIT_DISPLAY_COLUMN_COUNT       9
#define MICROBIT_DISPLAY_ROW_COUNT          3

    const MatrixPoint microbitDisplayMap[MICROBIT_DISPLAY_ROW_COUNT * MICROBIT_DISPLAY_COLUMN_COUNT] =
    {
        {0,4},{0,3},{1,1},
        {1,4},{4,2},{0,1},
        {2,4},{3,2},{4,0},
        {3,4},{2,2},{3,0},
        {4,4},{1,2},{2,0},
        {4,3},{0,2},{1,0},
        {3,3},{4,1},{0,0},
        {2,3},{3,1},{NO_CONN,NO_CONN},
        {1,3},{2,1},{NO_CONN,NO_CONN}
    };

#endif

#if MICROBIT_DISPLAY_TYPE == MICROBIT_SB1

#define MICROBIT_DISPLAY_COLUMN_COUNT       3
#define MICROBIT_DISPLAY_ROW_COUNT          9

    const MatrixPoint microbitDisplayMap[MICROBIT_DISPLAY_ROW_COUNT * MICROBIT_DISPLAY_COLUMN_COUNT] =
    {
        {0,4},{1,4},{2,4},{3,4},{4,4},{4,3},{3,3},{2,3},{1,3},
        {0,3},{4,2},{3,2},{2,2},{1,2},{0,2},{4,1},{3,1},{2,1},
        {1,1},{0,1},{4,0},{3,0},{2,0},{1,0},{0,0},{NO_CONN,NO_CONN},{NO_CONN,NO_CONN}
    };

#endif

#if MICROBIT_DISPLAY_TYPE == MICROBIT_SB2

#define MICROBIT_DISPLAY_COLUMN_COUNT       9
#define MICROBIT_DISPLAY_ROW_COUNT          3

    const MatrixPoint microbitDisplayMap[MICROBIT_DISPLAY_ROW_COUNT * MICROBIT_DISPLAY_COLUMN_COUNT] =
    {
        {0,0},{4,2},{2,4},
        {2,0},{0,2},{4,4},
        {4,0},{2,2},{0,4},
        {4,3},{1,0},{0,1},
        {3,3},{3,0},{1,1},
        {2,3},{3,4},{2,1},
        {1,3},{1,4},{3,1},
        {0,3},{NO_CONN,NO_CONN},{4,1},
        {1,2},{NO_CONN,NO_CONN},{3,2}
    };

#endif

//ROW1 and COL1 are defined in mbed classic:
//https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/targets/hal/TARGET_NORDIC/TARGET_MCU_NRF51822/TARGET_NRF51_MICROBIT/PinNames.h
const MatrixMap microbitMatrixMap =
{
    MICROBIT_DISPLAY_WIDTH,
    MICROBIT_DISPLAY_HEIGHT,
    MICROBIT_DISPLAY_ROW_COUNT,
    MICROBIT_DISPLAY_COLUMN_COUNT,
    MICROBIT_DISPLAY_ROW1,
    MICROBIT_DISPLAY_COL1,
    microbitDisplayMap
};

#endif
