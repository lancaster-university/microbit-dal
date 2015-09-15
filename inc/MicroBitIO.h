#ifndef MICROBIT_IO_H
#define MICROBIT_IO_H

#include "mbed.h"
#include "MicroBitComponent.h"
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
    MicroBitIO(int ID_P0, int ID_P1, int ID_P2,
               int ID_P3, int ID_P4, int ID_P5,
               int ID_P6, int ID_P7, int ID_P8,
               int ID_P9, int ID_P10,int ID_P11,
               int ID_P12,int ID_P13,int ID_P14,
               int ID_P15,int ID_P16,int ID_P19,
               int ID_P20);
};

#endif

