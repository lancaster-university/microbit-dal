#include "PeridoRadioCloud.h"
#include "MicroBitPeridoRadio.h"
#include "MicroBitCompat.h"
#include "ErrorNo.h"
#include "MicroBitEvent.h"

PeridoRadioVariable::PeridoRadioVariable(PeridoRadioCloud& cloud) : cloud(cloud)
{

}

int PeridoRadioVariable::setVariable(CloudVariable& c)
{
    DynamicType t;

    t.appendInteger(c.variableNamespaceHash);
    t.appendInteger(c.variableNameHash);
    t.appendString(c.value);

    uint32_t len = min(MAX_PAYLOAD_SIZE, t.length());

    uint8_t* buf = (uint8_t*) malloc(len);
    memcpy(buf, t.getBytes(), len);

    int ret = cloud.send(REQUEST_TYPE_CLOUD_VARIABLE, buf, len);
    free(buf);

    return ret;
}

void PeridoRadioVariable::handleTimeout(uint16_t id)
{
    // just remove for now, but should retransmit last change in future...
    CloudDataItem* t = cloud.recvRaw(id);

    if (t)
        delete t;
}

void PeridoRadioVariable::handlePacket(uint16_t id)
{
    // use the raw variant and coherce into DynamicType
    CloudDataItem* c = cloud.recvRaw(id);
    DataPacket* t = (DataPacket*)c->packet->payload;

    if (c && !(t->request_type & (REQUEST_STATUS_OK | REQUEST_STATUS_ERROR)))
    {
        DynamicType dt(c->packet->length - MICROBIT_PERIDO_HEADER_SIZE - CLOUD_HEADER_SIZE, t->payload, 0);
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

        delete c;
    }
}