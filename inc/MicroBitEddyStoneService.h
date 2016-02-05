#ifndef MICROBIT_EDDYSTONE_SERVICE_H
#define MICROBIT_EDDYSTONE_SERVICE_H

#include "MicroBit.h"

#define EDDYSTONE_NUM_PREFIXES          4
#define EDDYSTONE_NUM_SUFFIXES          14
#define EDDYSTONE_NUM_EDDYSTONE_FRAMES  3

//TODO: move to config.h
#define BEACON_DEFAULT_URL              "https://www.microbit.co.uk"

#define EDDYSTONE_BEACON_DEFAULT_NS     "microbit"

#define EDDYSTONE_URL_DATA_MAX          18
#define EDDYSTONE_URL_DATA_MIN_LEN      12

#define EDDYSTONE_FRAME_TYPE_UID        0x00
#define EDDYSTONE_FRAME_TYPE_URL        0x10
#define EDDYSTONE_FRAME_TYPE_TLM        0x20

#define EDDYSTONE_FRAME_SIZE_UID        20
#define EDDYSTONE_FRAME_SIZE_URL        2
#define EDDYSTONE_FRAME_SIZE_TLM        14

#define EDDYSTONE_HEADER_SIZE           2
#define EDDYSTONE_UID_RESERVED_SIZE     2
#define EDDYSTONE_NAMESPACE_SIZE        10
#define EDDYSTONE_INSTANCE_SIZE         6

#define EDDYSTONE_FRAME_UID             0
#define EDDYSTONE_FRAME_URL             1
#define EDDYSTONE_FRAME_TLM             2


const uint8_t EDDYSTONE_UUID[] = {0xAA, 0xFE};

struct UIDFrame
{

    private:
    ManagedString namespaceID;
    ManagedString instanceID;

    public:
    UIDFrame(ManagedString namespaceID, ManagedString instanceID)
    {
        this->namespaceID = namespaceID;
        this->instanceID = instanceID;
    }

    void getFrame(uint8_t * frameBuf)
    {
        int index = 0;

        frameBuf[index++] = EDDYSTONE_UUID[0];
        frameBuf[index++] = EDDYSTONE_UUID[1];


        frameBuf[index++] = EDDYSTONE_FRAME_TYPE_UID;
        frameBuf[index++] = 0;

        int namespaceIndex = 0;
        int instanceIndex = 0;

        while(namespaceIndex < EDDYSTONE_NAMESPACE_SIZE)
        {
            char c = 0;

            if(namespaceIndex < this->namespaceID.length())
                c = this->namespaceID.charAt(namespaceIndex);

            frameBuf[index++] = c;
            namespaceIndex++;
        }

        while(instanceIndex < EDDYSTONE_INSTANCE_SIZE)
        {
            char c = 0;

            if(instanceIndex < this->instanceID.length())
                c = this->instanceID.charAt(instanceIndex);

            frameBuf[index++] = c;
            instanceIndex++;
        }

        int reservedCount = 0;

        while(reservedCount < EDDYSTONE_UID_RESERVED_SIZE)
        {
            frameBuf[index++] = 0;
            reservedCount++;
        }

#if CONFIG_ENABLED(MICROBIT_DBG)

        uBit.serial.printf("uid %d [%d]... ");

        for(int i = 0; i < length(); i++)
        {
            uBit.serial.printf("%d,",frameBuf[i]);
        }

        uBit.serial.printf("\r\n");
#endif
    }

    int length()
    {
        return EDDYSTONE_HEADER_SIZE + EDDYSTONE_FRAME_SIZE_UID;
    }
};


struct TLMFrame
{
    public:
    TLMFrame()
    {
    }

    void getFrame(uint8_t * frameBuf)
    {
        int index = 0;
        uint32_t tempTicks = ticks;
        uint16_t temperature = uBit.thermometer.getTemperature();

        frameBuf[index++] = EDDYSTONE_UUID[0];
        frameBuf[index++] = EDDYSTONE_UUID[1];

        frameBuf[index++] = EDDYSTONE_FRAME_TYPE_TLM;           // Eddystone frameBuf type = Telemetry
        frameBuf[index++] = 0;                             // TLM Version Number
        frameBuf[index++] = 0;                             // Battery Voltage[0]
        frameBuf[index++] = 0;                             // Battery Voltage[1]
        frameBuf[index++] = (uint8_t)(temperature >> 8);   // Beacon Temp[0]
        frameBuf[index++] = (uint8_t)(temperature >> 0);   // Beacon Temp[1]
        frameBuf[index++] = 0;                             // PDU Count [0]
        frameBuf[index++] = 0;                             // PDU Count [1]
        frameBuf[index++] = 0;                             // PDU Count [2]
        frameBuf[index++] = 0;                             // PDU Count [3]
        frameBuf[index++] = (uint8_t)(tempTicks >> 24);    // Time Since Boot [0]
        frameBuf[index++] = (uint8_t)(tempTicks >> 16);    // Time Since Boot [1]
        frameBuf[index++] = (uint8_t)(tempTicks >> 8);     // Time Since Boot [2]
        frameBuf[index++] = (uint8_t)(tempTicks >> 0);     // Time Since Boot [3]

#if CONFIG_ENABLED(MICROBIT_DBG)

        uBit.serial.printf("uid... ");

        for(int i = 0; i < length(); i++)
        {
            uBit.serial.printf("%d,",frameBuf[i]);
        }

        uBit.serial.printf("\r\n");
#endif
    }

    int length()
    {
        return EDDYSTONE_HEADER_SIZE + EDDYSTONE_FRAME_SIZE_TLM;
    }
};

struct URLFrame
{
    ManagedString encodedURL;

    ManagedString encodeURL(ManagedString url)
    {

        //TODO: THIS WILL FAIL IF prefix or suffix index == 0!!!

        ManagedString defaultURL = ManagedString(BEACON_DEFAULT_URL);

        ManagedString prefixes[EDDYSTONE_NUM_PREFIXES] = {
            "http://www.",
            "https://www.",
            "http://",
            "https://",
        };

        ManagedString suffixes[EDDYSTONE_NUM_SUFFIXES]   = {
            ".com/",
            ".org/",
            ".edu/",
            ".net/",
            ".info/",
            ".biz/",
            ".gov/",
            ".com",
            ".org",
            ".edu",
            ".net",
            ".info",
            ".biz",
            ".gov"
        };

        //ensure we have a valid string
        if(url.length() == 0 || url.length() < EDDYSTONE_URL_DATA_MIN_LEN)
            url = defaultURL;

        uint8_t urlEncoded[EDDYSTONE_URL_DATA_MAX] = { 0 };

        int urlDataLength = 0;

        int stringPos = 0;

        //iterate through the prefixes array attempting to find a match
        for (int prefixCount = 0; prefixCount < EDDYSTONE_NUM_PREFIXES; prefixCount++)
        {
            int prefixLength = prefixes[prefixCount].length();

            if (url.substring(stringPos, prefixLength) == prefixes[prefixCount])
            {
                urlEncoded[urlDataLength++] = prefixCount;
                stringPos += prefixLength;
                break;
            }
        }

        while (stringPos < url.length() && urlDataLength < EDDYSTONE_URL_DATA_MAX )
        {
            int suffixCount = 0;

            //iterate through the suffixes array attempting to find a match
            for (suffixCount = 0; suffixCount < EDDYSTONE_NUM_SUFFIXES; suffixCount++)
            {
                int suffixLength = suffixes[suffixCount].length();

                if (url.substring(stringPos, suffixLength) == suffixes[suffixCount])
                {
                    urlEncoded[urlDataLength++]  = suffixCount;
                    stringPos += suffixLength;
                    break;
                }
            }

            //if we haven't matched a suffix, store the "ordinary character"
            if (suffixCount == EDDYSTONE_NUM_SUFFIXES)
                urlEncoded[urlDataLength++] = url.charAt(stringPos++);
        }

        return ManagedString((char *)urlEncoded, urlDataLength);
    };

public:
    URLFrame(ManagedString url)
    {
        this->encodedURL = encodeURL(url);
    }

    void getFrame(uint8_t * frameBuf)
    {
        int index = 0;

        frameBuf[index++] = EDDYSTONE_UUID[0];
        frameBuf[index++] = EDDYSTONE_UUID[1];

        frameBuf[index++] = EDDYSTONE_FRAME_TYPE_URL;
        frameBuf[index++] = 0;

        for(int urlIndex = 0; urlIndex < this->encodedURL.length(); urlIndex++)
            frameBuf[index++] = this->encodedURL.charAt(urlIndex);

#if CONFIG_ENABLED(MICROBIT_DBG)

        uBit.serial.printf("uid... ");

        for(int i = 0; i < length(); i++)
        {
            uBit.serial.printf("%d,",frameBuf[i]);
        }

        uBit.serial.printf("\r\n");
#endif
    }

    void setURL(ManagedString url)
    {
        this->encodedURL = encodeURL(url);
    }

    int length()
    {
        return EDDYSTONE_HEADER_SIZE + EDDYSTONE_FRAME_SIZE_URL + this->encodedURL.length();
    }
};


/**
  * Class definition for a MicroBit BLE Temperture Service.
  * Provides access to live temperature data via BLE.
  */
class MicroBitEddyStoneService
{
    // Bluetooth stack we're running on.
    BLEDevice &ble;

    UIDFrame uidFrame;
    URLFrame urlFrame;
    TLMFrame tlmFrame;

    ManagedString namespaceID;
    ManagedString instance;

    uint8_t currentFrame;

    public:

    /**
      * Constructor.
      * Create a representation of the TempertureService
      * @param _ble The instance of a BLE device that we're running on.
      */
    MicroBitEddyStoneService(BLEDevice &_ble, ManagedString url, ManagedString namespaceID = ManagedString(EDDYSTONE_BEACON_DEFAULT_NS), ManagedString instanceID = ManagedString(uBit.getName()));


    uint16_t getUIDEncoded();

    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    void onDataWritten(const GattWriteCallbackParams *params);

    void radioNotificationCallback(bool radioActive);

    void updateAdvertisementPacket();
};


#endif
