/*
The MIT License (MIT)

Copyright (c) 2016 Lancaster University, UK.

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

#include "MicroBitConfig.h"
#include "TimedInterruptIn.h"
/**
  * Constructor.
  *
  * Create an instance of TimedInterruptIn that has an additional timestamp field.
  */
TimedInterruptIn::TimedInterruptIn(PinName name) : InterruptIn(name)
{
    timestamp = 0;
}

/**
  * Stores the given timestamp for this instance of TimedInterruptIn.
  *
  * @param timestamp the timestamp to retain.
  */
void TimedInterruptIn::setTimestamp(uint64_t timestamp)
{
    this->timestamp = timestamp;
}

/**
  * Retrieves the retained timestamp for this instance of TimedInterruptIn.
  *
  * @return the timestamp held by this instance.
  */
uint64_t TimedInterruptIn::getTimestamp()
{
    return timestamp;
}
