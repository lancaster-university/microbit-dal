#ifndef MICROBIT_IO_H
#define MICROBIT_IO_H

#include "mbed.h"
#include "MicroBitPin.h"

/**
  * Class definition for MicroBit IO.
  *
  * This is an object that contains the pins on the edge connector as properties.
  */
class MicroBitIO
{   
    public:
    
    MicroBitPin          P0;
    MicroBitPin          P1;
    MicroBitPin          P2;
    MicroBitPin          P3;
    MicroBitPin          P4;
    MicroBitPin          P5;
    MicroBitPin          P6;
    MicroBitPin          P7;
    MicroBitPin          P8;
    MicroBitPin          P9;
    MicroBitPin          P10;
    MicroBitPin          P11;
    MicroBitPin          P12;
    MicroBitPin          P13;
    MicroBitPin          P14;
    MicroBitPin          P15;
    MicroBitPin          P16;
    MicroBitPin          P19;
    MicroBitPin          P20;
    
    /**
      * Constructor. 
      * Create a representation of all given I/O pins on the edge connector
      */
    MicroBitIO(int MICROBIT_ID_IO_P0, int MICROBIT_ID_IO_P1, int MICROBIT_ID_IO_P2,
               int MICROBIT_ID_IO_P3, int MICROBIT_ID_IO_P4, int MICROBIT_ID_IO_P5,
               int MICROBIT_ID_IO_P6, int MICROBIT_ID_IO_P7, int MICROBIT_ID_IO_P8,
               int MICROBIT_ID_IO_P9, int MICROBIT_ID_IO_P10,int MICROBIT_ID_IO_P11,
               int MICROBIT_ID_IO_P12,int MICROBIT_ID_IO_P13,int MICROBIT_ID_IO_P14,
               int MICROBIT_ID_IO_P15,int MICROBIT_ID_IO_P16,int MICROBIT_ID_IO_P19,
               int MICROBIT_ID_IO_P20);
    
    
};

#endif

