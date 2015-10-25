#include "mbed.h"
#include "MicroBit.h"

void RefCounted::init()
{
    // Initialize to one reference (lowest bit set to 1)
    refCount = 3;
}

static inline bool isReadOnlyInline(RefCounted *t)
{
    uint32_t refCount = t->refCount;

    if (refCount == 0xffff)
        return true; // object in flash

    // Do some sanity checking while we're here
    if (refCount == 1)
        uBit.panic(30); // object should have been deleted

    if ((refCount & 1) == 0)
        uBit.panic(31); // refCount doesn't look right

    // Not read only
    return false;
}

bool RefCounted::isReadOnly()
{
    return isReadOnlyInline(this);
}

void RefCounted::incr()
{
    if (!isReadOnlyInline(this))
        refCount += 2;
}

void RefCounted::decr()
{
    if (isReadOnlyInline(this))
        return;

    refCount -= 2;
    if (refCount == 2) {
        free(this);
    }
}
