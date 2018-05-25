#ifndef CLOUD_VARIABLE_H
#define CLOUD_VARIABLE_H

#include "MicroBitComponent.h"
#include "ManagedString.h"

class Radio;

#define CLOUD_VARIABLE_MAX_VARIABLES    10

class CloudVariable
{
    Radio& radio;

    public:

    uint16_t variableNameHash;
    uint16_t variableNamespaceHash;

    ManagedString value;

    static CloudVariable* variables[CLOUD_VARIABLE_MAX_VARIABLES];

    CloudVariable(ManagedString variableNamespace, ManagedString variableName, Radio& radio);

    void operator= (const ManagedString &value);

    static uint16_t pearsonHash(ManagedString s);

    ~CloudVariable();

    friend class RadioVariable;
};

#endif

