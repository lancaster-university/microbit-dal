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
  * Class definition for a MemberFunctionCallback.
  *
  * C++ member functions (also known as methods) have a more complex
  * representation than normal C functions. This class allows a reference to
  * a C++ member function to be stored then called at a later date.
  *
  * This class is used extensively by the MicroBitMessageBus to deliver
  * events to C++ methods.
  */

#include "MicroBitConfig.h"
#include "MemberFunctionCallback.h"

/**
  * Calls the method reference held by this MemberFunctionCallback.
  *
  * @param e The event to deliver to the method
  */
void MemberFunctionCallback::fire(MicroBitEvent e)
{
    invoke(object, method, e);
}

/**
  * A comparison of two MemberFunctionCallback objects.
  *
  * @return true if the given MemberFunctionCallback is equivalent to this one, false otherwise.
  */
bool MemberFunctionCallback::operator==(const MemberFunctionCallback &mfc)
{
    return (object == mfc.object && (memcmp(method,mfc.method,sizeof(method))==0));
}
