#include "DynamicType.h"
#include "ErrorNo.h"

extern void log_string(const char *);
extern void log_num(int);

void DynamicType::init(uint8_t len, uint8_t* payload, bool resize)
{
    if (resize)
        free(this->ptr);

    ptr = (SubTyped *) malloc(sizeof(SubTyped) + len);
    ptr->init();

    ptr->len = len;

    // Copy in the data buffer, if provided.
    if (len > 0)
        memcpy(ptr->payload, payload, len);
}

DynamicType::DynamicType()
{
    status = DYNAMIC_TYPE_STATUS_NOT_CONFIGURED;
    this->init(0, NULL);
}

DynamicType::DynamicType(uint8_t len, uint8_t* payload, uint8_t status)
{
    status = (status > 0) ? DYNAMIC_TYPE_STATUS_ERROR : MICROBIT_OK;
    this->init(len, payload);
}

DynamicType::DynamicType(const DynamicType &buffer)
{
    ptr = buffer.ptr;
    status = buffer.status;
    ptr->incr();
}

DynamicType::~DynamicType()
{
    ptr->decr();
}

DynamicType& DynamicType::operator = (const DynamicType &p)
{
    if(ptr == p.ptr)
        return *this;

    ptr->decr();
    ptr = p.ptr;
    ptr->incr();

    return *this;
}

uint8_t* DynamicType::getPointerToIndex(int index)
{
    uint8_t* payloadPtr = ptr->payload;
    uint8_t* payloadEnd = ptr->payload + ptr->len;

    for (int i = 0; i < index; i++)
    {
        if (payloadPtr >= payloadEnd)
            return NULL;

        uint8_t subtype =  *payloadPtr++;

        if (subtype & SUBTYPE_STRING)
            while(*payloadPtr++ != 0);

        if (subtype & SUBTYPE_INT)
            payloadPtr += sizeof(int);
        
        if (subtype & SUBTYPE_FLOAT)
            payloadPtr += sizeof(float);
    }

    return payloadPtr;
}

uint8_t* DynamicType::getBytes()
{
    return ptr->payload;
}

int DynamicType::length()
{
    return ptr->len;
}

int DynamicType::getStatus()
{
    if (status & (DYNAMIC_TYPE_STATUS_NOT_CONFIGURED) || status == 0)
        return MICROBIT_OK;
    
    return MICROBIT_NO_DATA;
}

ManagedString DynamicType::getString(int index)
{
    uint8_t *data = getPointerToIndex(index);

    if (data == NULL || !(*data & SUBTYPE_STRING))
        return ManagedString();
    
    // move past subtype byte
    data++;
    
    return ManagedString((char*)data);
}

int DynamicType::getInteger(int index)
{
    uint8_t *data = getPointerToIndex(index);

    if (data == NULL || !(*data & SUBTYPE_INT))
        return MICROBIT_INVALID_PARAMETER;
    
    // move past subtype byte
    data++;
    
    return (int)*data;
}

float DynamicType::getFloat(int index)
{
    uint8_t *data = getPointerToIndex(index);

    if (data == NULL || !(*data & SUBTYPE_FLOAT))
        return MICROBIT_INVALID_PARAMETER;
    
    // move past subtype byte
    data++;
    
    return (float)*data;
}

int DynamicType::grow(uint8_t size, uint8_t subtype, uint8_t* data)
{
    int len = length();
    int newSize = len + size + 1;

    uint8_t *buf = (uint8_t *)malloc(newSize); // include subtype byte

    if (buf == NULL)
        return MICROBIT_NO_RESOURCES;

    memcpy(buf, getBytes(), len);
    
    // set data type
    buf[len] = subtype;

    // append the new string
    memcpy(buf+ len + 1, data, size);

    this->init(newSize, buf, true);

    return MICROBIT_OK;
}

int DynamicType::appendString(ManagedString s)
{
    return grow(s.length() + 1, SUBTYPE_STRING, (uint8_t *)s.toCharArray());
}

int DynamicType::appendInteger(int i)
{
    return grow(sizeof(int), SUBTYPE_INT, (uint8_t *)&i);
}

int DynamicType::appendFloat(float f)
{
    return grow(sizeof(float), SUBTYPE_FLOAT, (uint8_t *)&f);
}
