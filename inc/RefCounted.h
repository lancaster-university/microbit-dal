#ifndef REF_COUNTED_H
#define REF_COUNTED_H

#include "mbed.h"

/**
  * Base class for payload for ref-counted objects. Used by ManagedString and MicroBitImage.
  * There is no constructor, as this struct is typically malloc()ed.
  */
struct RefCounted
{
public:
    /**
      * Number of outstanding references. Should never be zero (object should be deleted then).
      * When it's set to 0xffff, it means the object sits in flash and should not be counted.
      */
    uint16_t refcnt;

    /**
      * For strings this is length. For images this is both width and length (8 bit each).
      * A value of 0xffff indicates that this is in fact an instance of VirtualRefCounted
      * and therefore when the reference count reaches zero, the virtual destructor
      * should be called.
      */
    union {
        uint16_t size;
        struct {
            uint8_t width;
            uint8_t height;
        };
    };

    /** Increment reference count. */
    void incr();

    /** Decrement reference count. */
    void decr();

};

#endif
