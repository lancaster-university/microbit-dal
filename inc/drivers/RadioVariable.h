#ifndef RADIO_VARIABLE_H
#define RADIO_VARIABLE_H

#include "RadioCloud.h"
#include "CloudVariable.h"

#include "DynamicType.h"
#include "MicroBitConfig.h"

class RadioVariable
{
    RadioCloud& cloud;

    public:

    RadioVariable(RadioCloud& cloud);

    int setVariable(CloudVariable& c);

    void handlePacket(DataPacket* temp);
};


#endif