#ifndef MEMBER_FUNCTION_CALLBACK_H
#define MEMBER_FUNCTION_CALLBACK_H

#include "mbed.h"
#include "MicroBitEvent.h"

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
     * @param object The object the callback method should be invooked on.
     * @param method The method to invoke.
     */
    template <typename T> MemberFunctionCallback(T* object, void (T::*method)(MicroBitEvent e));

    /**
     * Comparison of two MemberFunctionCallback objects.
     * @return TRUE if the given MemberFunctionCallback is equivalent to this one. FALSE otherwise.
     */
    bool operator==(const MemberFunctionCallback &mfc);

    /**
     * Calls the method reference held by this MemberFunctionCallback.
     * @param e The event to deliver to the method
     */
    void fire(MicroBitEvent e);
};

/**
 * Constructor. Creates a representation of a pointer to a C++ member function (method). 
 * @param object The object the callback method should be invooked on.
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
  * Template to create static methods capable of invoking a C++ member function (method)
  * based on the given paramters.
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


