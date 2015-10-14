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

