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

#ifndef MEMBER_FUNCTION_CALLBACK_H
#define MEMBER_FUNCTION_CALLBACK_H

#include "mbed.h"
#include "MicroBitConfig.h"
#include "MicroBitEvent.h"
#include "MicroBitCompat.h"

/**
  * Class definition for a MemberFunctionCallback.
  *
  * C++ member functions (also known as methods) have a more complex
  * representation than normal C functions. This class allows a reference to
  * a C++ member function to be stored then called at a later date.
  *
  * This class is used extensively by the MicroBitMessageBus to deliver
  * events to C++ methods.
  */
class MemberFunctionCallback
{
    private:
    void* object;
    uint32_t method[4];
    void (*invoke)(void *object, uint32_t *method, MicroBitEvent e);
    template <typename T> static void methodCall(void* object, uint32_t*method, MicroBitEvent e);

    public:

    /**
      * Constructor. Creates a MemberFunctionCallback based on a pointer to given method.
      *
      * @param object The object the callback method should be invooked on.
      *
      * @param method The method to invoke.
      */
    template <typename T> MemberFunctionCallback(T* object, void (T::*method)(MicroBitEvent e));

    /**
      * A comparison of two MemberFunctionCallback objects.
      *
      * @return true if the given MemberFunctionCallback is equivalent to this one, false otherwise.
      */
    bool operator==(const MemberFunctionCallback &mfc);

    /**
      * Calls the method reference held by this MemberFunctionCallback.
      *
      * @param e The event to deliver to the method
      */
    void fire(MicroBitEvent e);
};

/**
  * Constructor. Creates a MemberFunctionCallback based on a pointer to given method.
  *
  * @param object The object the callback method should be invooked on.
  *
  * @param method The method to invoke.
  */
template <typename T>
MemberFunctionCallback::MemberFunctionCallback(T* object, void (T::*method)(MicroBitEvent e))
{
    this->object = object;
    memclr(this->method, sizeof(this->method));
    memcpy(this->method, &method, sizeof(method));
    invoke = &MemberFunctionCallback::methodCall<T>;
}

/**
  * A template used to create a static method capable of invoking a C++ member function (method)
  * based on the given parameters.
  *
  * @param object The object the callback method should be invooked on.
  *
  * @param method The method to invoke.
  *
  * @param method The MicroBitEvent to supply to the given member function.
  */
template <typename T>
void MemberFunctionCallback::methodCall(void *object, uint32_t *method, MicroBitEvent e)
{
    T* o = (T*)object;
    void (T::*m)(MicroBitEvent);
    memcpy(&m, method, sizeof(m));

    (o->*m)(e);
}

#endif
