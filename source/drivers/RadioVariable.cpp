#include "RadioVariable.h"

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
    p->app_id = cloud.app_id;
    p->id = id;
    p->request_type = REQUEST_TYPE_CLOUD_VARIABLE;
    p->status = DATA_PACKET_WAITING_FOR_SEND;

    uint32_t len = min(MAX_PAYLOAD_SIZE, t.length());

    p->len = len;

    memcpy(p->payload, t.getBytes(), len);

    cloud.addToTxQueue(p);
}

void RadioVariable::handlePacket(DataPacket* temp)
{
    // use the raw variant and coherce into DynamicType
    DataPacket* t = cloud.recvRaw(temp->id);

    // can't do much if there's an error from a previous request
    if (!(t->request_type & REQUEST_TYPE_STATUS_ERROR))
    {
        dt = DynamicType(t->len - CLOUD_HEADER_SIZE, t->payload, 0);

        for (int i = 0; i < CLOUD_VARIABLE_MAX_VARIABLES; i++)
        {
            if (CloudVariable::variables[i] != NULL)
            {
                if (CloudVariable::variables[i].variableNamespaceHash == t.getInteger(0) && CloudVariable::variables[i].variableNameHash == t.getInteger(1))
                {
                    CloudVariable::variables[i].value = t.getString(2);
                    MicroBitEvent(RADIO_CLOUD_VARIABLE_ID, variableNameHash);
                    break;
                }
            }
        }
    }

    delete t;
}