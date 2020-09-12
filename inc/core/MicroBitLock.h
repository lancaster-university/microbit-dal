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
 * A simple lock, mostly used for mutual exclusion.
 */

#ifndef MICROBIT_LOCK_H
#define MICROBIT_LOCK_H

#include "MicroBitConfig.h"

class Fiber;

class MicroBitLock
{
    private:
    bool    locked;
    Fiber   *queue;

    public:

    /**
     * Create a new lock that can be used for mutual exclusion and condition synchronisation.
     */
    MicroBitLock();

    /**
     * Block the calling fiber until the lock is available
     **/
    void wait();

    /**
     * Release the lock, and signal to one waiting fiber to continue
     */
    void notify();

    /**
     * Release the lock, and signal to all waiting fibers to continue
     */
    void notifyAll();
};

#endif
