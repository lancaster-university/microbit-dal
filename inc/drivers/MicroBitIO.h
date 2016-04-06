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

#ifndef MICROBIT_IO_H
#define MICROBIT_IO_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitComponent.h"
#include "MicroBitPin.h"

/**
  * Class definition for MicroBit IO.
  *
  * Represents a collection of all I/O pins on the edge connector.
  */
class MicroBitIO
{
    public:

	MicroBitPin			 pin[0];
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
      *
      * Create a representation of all given I/O pins on the edge connector
      *
      * Accepts a sequence of unique ID's used to distinguish events raised
      * by MicroBitPin instances on the default EventModel.
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
