#include "mbed.h"
#include "MicroBit.h"

void RefCounted::incr()
{
    if (refcnt == 0xffff)
        return;
    if (refcnt == 0)
        uBit.panic(30);
    refcnt++;
}

void RefCounted::decr()
{
    if (refcnt == 0xffff)
        return;
    if (refcnt == 0)
        uBit.panic(31);
    refcnt--;
    if (refcnt == 0) {
        // size of 0xffff indicates a class with a virtual destructor
        if (size == 0xffff)
            uBit.panic(32);
        else
            free(this);
    }
}
