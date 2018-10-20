#ifndef PERIDO_RADIO_VARIABLE_H
#define PERIDO_RADIO_VARIABLE_H

class PeridoRadioCloud;

#include "CloudVariable.h"
#include "DynamicType.h"
#include "MicroBitConfig.h"

class PeridoRadioVariable
{
    PeridoRadioCloud& cloud;

    public:

    PeridoRadioVariable(PeridoRadioCloud& cloud);

    int setVariable(CloudVariable& c);

    void handleTimeout(uint16_t id);
    void handlePacket(uint16_t id);
};


#endif