#ifndef AES_128_H
#define AES_128_H

#include "MicroBitConfig.h"

#define AES128_BYTE_LEN     16

struct AES_CCM
{
    uint8_t key[AES128_BYTE_LEN];
    uint32_t packet_counter;
    uint8_t gap;
    uint16_t maxPacketSize;
    uint8_t direction;
    uint8_t iv;
};


struct AES_Encrypt
{
    uint8_t header;
    uint8_t length;
    uint8_t rfu;
    uint8_t payload[32];
};

class AES128
{
    AES_CCM ccm;

    int encryptDecrypt(AES_Encrypt* src, AES_Encrypt* destination, bool encrypt);

    public:
    AES128(uint8_t* key, uint8_t iv, uint32_t maxPacketSize, int keyLen = AES128_BYTE_LEN);

    int encrypt(AES_Encrypt* src, AES_Encrypt* destination);
    int decrypt(AES_Encrypt* src, AES_Encrypt* destination);

    int enable();
    int disable();
};


#endif