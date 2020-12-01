/*
The MIT License (MIT)

Copyright (c) 2020 Insight Resources

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

#ifndef MICROBIT_TICK64_H
#define MICROBIT_TICK64_H

#include "mbed.h"
#include "MicroBitConfig.h"


typedef struct microbit_tick64_t
{
  uint32_t low0;  // microseconds
  uint32_t high;  // microbit_tick64_highunit microseconds
} microbit_tick64_t;


extern microbit_tick64_t  microbit_tick640;
extern microbit_tick64_t  microbit_tick641;
extern microbit_tick64_t *microbit_tick64;
extern const uint32_t     microbit_tick64_highunit;


inline void microbit_tick64_initialise()
{
    microbit_tick64 = &microbit_tick640;
    microbit_tick64->low0 = us_ticker_read();
    microbit_tick64->high = 0;
}


/**
 * Check the microsecond ticker and swap microbit_tick64_t instances
 * Not reentrant. Call from one place. e.g. in a ticker.
 * Assumes the microsecond ticker difference is less than
 * 2 * microbit_tick64_highunit and calculates new values when
 * the difference is between microbit_tick64_highunit and 0xFFFFFFF.
 * With microbit_tick64_highunit = 0x80000000, the check is needed
 * every 30mins and rollover is after 250,000 years
*/
inline void microbit_tick64_update()
{
    if ( us_ticker_read() - microbit_tick64->low0 > microbit_tick64_highunit)
    {
        microbit_tick64_t *other = microbit_tick64 == &microbit_tick640 ? &microbit_tick641 : &microbit_tick640;
        other->low0 = microbit_tick64->low0 + microbit_tick64_highunit;
        other->high = microbit_tick64->high + 1;
        microbit_tick64 = other;
    }
}


inline uint64_t microbit_tick64_microseconds()
{
    microbit_tick64_t *p = microbit_tick64; // guard against IRQ changing microbit_tick64
    return microbit_tick64_highunit * ( uint64_t) p->high + ( us_ticker_read() - p->low0);
}


#endif
