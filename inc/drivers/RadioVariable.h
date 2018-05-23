#ifndef RADIO_VARIABLE_H
#define RADIO_VARIABLE_H

class RadioCloud;

#include "CloudVariable.h"
#include "DynamicType.h"
#include "MicroBitConfig.h"

class RadioVariable
{
    RadioCloud& cloud;

    public:

    RadioVariable(RadioCloud& cloud);

    int setVariable(CloudVariable& c);

    void handleTimeout(uint16_t id);
    void handlePacket(uint16_t id);
};


#endif