#include "RadioCloud.h"
#include "MicroBitCompat.h"
#include "ErrorNo.h"
#include "MicroBitEvent.h"

extern void log_string(const char *);
extern void log_num(int);

RadioVariable::RadioVariable(RadioCloud& cloud) : cloud(cloud)
{

}

int RadioVariable::setVariable(CloudVariable& c)
{
    DynamicType t;

    t.appendInteger(c.variableNamespaceHash);
    t.appendInteger(c.variableNameHash);
    t.appendString(c.value);

    uint32_t id = microbit_random(65535);

    DataPacket* p = new DataPacket();
    p->app_id = cloud.appId;
    p->id = id;
    p->request_type = REQUEST_TYPE_CLOUD_VARIABLE;
    p->status = DATA_PACKET_WAITING_FOR_SEND;

    uint32_t len = min(MAX_PAYLOAD_SIZE, t.length());

    p->len = len;

    memcpy(p->payload, t.getBytes(), len);

    cloud.addToTxQueue(p);

    return MICROBIT_OK;
}

void RadioVariable::handleTimeout(uint16_t id)
{
    // just remove for now, but should retransmit last change in future...
    DataPacket* t = cloud.recvRaw(id);
    delete t;
}

void RadioVariable::handlePacket(uint16_t id)
{
    // use the raw variant and coherce into DynamicType
    DataPacket* t = cloud.recvRaw(id);

    if (t == NULL)
        return;

    // can't do much if there's an error from a previous request
    if (!(t->request_type & REQUEST_TYPE_STATUS_ERROR))
    {
        DynamicType dt(t->len - CLOUD_HEADER_SIZE, t->payload, 0);
        uint16_t namespaceHash = dt.getInteger(0);
        uint16_t variableHash = dt.getInteger(1);

        for (int i = 0; i < CLOUD_VARIABLE_MAX_VARIABLES; i++)
        {
            if (CloudVariable::variables[i] != NULL)
            {
                if (CloudVariable::variables[i]->variableNamespaceHash == namespaceHash && CloudVariable::variables[i]->variableNameHash == variableHash)
                {
                    CloudVariable::variables[i]->value = dt.getString(2);
                    MicroBitEvent(RADIO_CLOUD_VARIABLE_ID, variableHash);
                    break;
                }
            }
        }
    }

    delete t;
}