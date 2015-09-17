/**
  * Class definition for a MemberFunctionCallback.
  *
  * C++ member functions (also known as methods) have a more complex
  * representation than normal C functions. This allows a referene to
  * a C++ member function to be stored then called at a later date.
  *
  * This class is used extensively by the MicroBitMessageBus to deliver
  * events to C++ methods.
  */

#include "MicroBit.h"

/**
  * Calls the method reference held by this MemberFunctionCallback.
  * @param e The event to deliver to the method
  */
void MemberFunctionCallback::fire(MicroBitEvent e)
{
    invoke(object, method, e);
}

/**
  * Comparison of two MemberFunctionCallback objects.
  * @return TRUE if the given MemberFunctionCallback is equivalent to this one. FALSE otherwise.
  */
bool MemberFunctionCallback::operator==(const MemberFunctionCallback &mfc)
{
    return (object == mfc.object && (memcmp(method,mfc.method,sizeof(method))==0)); 
}

