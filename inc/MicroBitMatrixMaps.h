/**
  * Definition of the LED Matrix maps supported.
  * Each map represents the layou of a different device.
  *
  * Ensure only one of these is selected.
  */

#ifndef MICROBIT_MATRIX_MAPS_H
#define MICROBIT_MATRIX_MAPS_H

/**
  * Provides the mapping from Matrix ROW/COL to a linear X/Y buffer. 
  * It's arranged such that matrixMap[col, row] provides the [x,y] screen co-ord.     
  */

#define NO_CONN 0

#ifdef MICROBUG_REFERENCE_DEVICE
    const MatrixPoint MicroBitDisplay::matrixMap[MICROBIT_DISPLAY_COLUMN_COUNT][MICROBIT_DISPLAY_ROW_COUNT] = 
    {   {MatrixPoint(0,0),MatrixPoint(0,1),MatrixPoint(0,2), MatrixPoint(0,3), MatrixPoint(0,4)},
        {MatrixPoint(1,0),MatrixPoint(1,1),MatrixPoint(1,2), MatrixPoint(1,3), MatrixPoint(1,4)},
        {MatrixPoint(2,0),MatrixPoint(2,1),MatrixPoint(2,2), MatrixPoint(2,3), MatrixPoint(2,4)},
        {MatrixPoint(3,0),MatrixPoint(3,1),MatrixPoint(3,2), MatrixPoint(3,3), MatrixPoint(3,4)},
        {MatrixPoint(4,0),MatrixPoint(4,1),MatrixPoint(4,2), MatrixPoint(4,3), MatrixPoint(4,4)} 
    };
#endif

#ifdef MICROBIT_3X9
    const MatrixPoint MicroBitDisplay::matrixMap[MICROBIT_DISPLAY_COLUMN_COUNT][MICROBIT_DISPLAY_ROW_COUNT] = 
    {   
        {MatrixPoint(0,4),MatrixPoint(0,3),MatrixPoint(1,1)},
        {MatrixPoint(1,4),MatrixPoint(4,2),MatrixPoint(0,1)},
        {MatrixPoint(2,4),MatrixPoint(3,2),MatrixPoint(4,0)},  
        {MatrixPoint(3,4),MatrixPoint(2,2),MatrixPoint(3,0)},
        {MatrixPoint(4,4),MatrixPoint(1,2),MatrixPoint(2,0)},
        {MatrixPoint(4,3),MatrixPoint(0,2),MatrixPoint(1,0)},
        {MatrixPoint(3,3),MatrixPoint(4,1),MatrixPoint(0,0)},
        {MatrixPoint(2,3),MatrixPoint(3,1),MatrixPoint(NO_CONN,NO_CONN)},
        {MatrixPoint(1,3),MatrixPoint(2,1),MatrixPoint(NO_CONN,NO_CONN)}
    };
#endif

#ifdef MICROBIT_SB1
    const MatrixPoint MicroBitDisplay::matrixMap[MICROBIT_DISPLAY_COLUMN_COUNT][MICROBIT_DISPLAY_ROW_COUNT] = 
    {   
        {MatrixPoint(0,4), MatrixPoint(1,4), MatrixPoint(2,4), MatrixPoint(3,4), MatrixPoint(4,4), MatrixPoint(4,3), MatrixPoint(3,3), MatrixPoint(2,3), MatrixPoint(1,3)},
        {MatrixPoint(0,3), MatrixPoint(4,2), MatrixPoint(3,2), MatrixPoint(2,2), MatrixPoint(1,2), MatrixPoint(0,2), MatrixPoint(4,1), MatrixPoint(3,1), MatrixPoint(2,1)},
        {MatrixPoint(1,1), MatrixPoint(0,1), MatrixPoint(4,0), MatrixPoint(3,0), MatrixPoint(2,0), MatrixPoint(1,0), MatrixPoint(0,0), MatrixPoint(NO_CONN,NO_CONN), MatrixPoint(NO_CONN,NO_CONN)}        
    };
#endif

#ifdef MICROBIT_SB2
    const MatrixPoint MicroBitDisplay::matrixMap[MICROBIT_DISPLAY_COLUMN_COUNT][MICROBIT_DISPLAY_ROW_COUNT] = 
    {   
        {MatrixPoint(0,0),MatrixPoint(4,2),MatrixPoint(2,4)},
        {MatrixPoint(2,0),MatrixPoint(0,2),MatrixPoint(4,4)},
        {MatrixPoint(4,0),MatrixPoint(2,2),MatrixPoint(0,4)},  
        {MatrixPoint(4,3),MatrixPoint(1,0),MatrixPoint(0,1)},
        {MatrixPoint(3,3),MatrixPoint(3,0),MatrixPoint(1,1)},
        {MatrixPoint(2,3),MatrixPoint(3,4),MatrixPoint(2,1)},
        {MatrixPoint(1,3),MatrixPoint(1,4),MatrixPoint(3,1)},
        {MatrixPoint(0,3),MatrixPoint(NO_CONN,NO_CONN),MatrixPoint(4,1)},
        {MatrixPoint(1,2),MatrixPoint(NO_CONN,NO_CONN),MatrixPoint(3,2)}
    };
    
    
#endif

const PinName rowPins[MICROBIT_DISPLAY_ROW_COUNT] = {MICROBIT_DISPLAY_ROW_PINS};
const uint8_t panicFace[MICROBIT_DISPLAY_COLUMN_COUNT] = {0x1B, 0x1B,0x0,0x0E,0x11};

#endif  

