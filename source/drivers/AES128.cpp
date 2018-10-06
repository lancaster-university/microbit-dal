#include "AES128.h"
#include "ErrorNo.h"

AES128::AES128(uint8_t* key, uint8_t iv, uint32_t maxPacketSize, int keyLen)
{
    memcpy(ccm.key, key, keyLen);
    ccm.iv = iv;
    ccm.maxPacketSize = maxPacketSize;

    NRF_CCM->CNFPTR = (uint32_t)&ccm;
}

int AES128::enable()
{
    NRF_CCM->ENABLE = 0x02;
    return MICROBIT_OK;
}

int AES128::disable()
{
    NRF_CCM->ENABLE = 0x00;
    return MICROBIT_OK;
}

int AES128::encryptDecrypt(AES_Encrypt* src, AES_Encrypt* destination, bool encrypt)
{
    uint8_t* scratch = (uint8_t*)malloc(16 + ccm.maxPacketSize);

    NRF_CCM->MICSTATUS = 0;

    NRF_CCM->MODE = (encrypt) ? 0x00 : 0x01;

    NRF_CCM->INPTR = (uint32_t)src;
    NRF_CCM->OUTPTR = (uint32_t)destination;
    NRF_CCM->SCRATCHPTR = (uint32_t)scratch;

    NRF_CCM->EVENTS_ENDCRYPT = 0;
    NRF_CCM->TASKS_CRYPT = 1;
    while(NRF_CCM->EVENTS_ENDCRYPT == 0);

    NRF_CCM->EVENTS_ENDCRYPT = 0;

    free(scratch);

    // message integrity check pass?
    if(!encrypt && NRF_CCM->MICSTATUS == 0)
        return MICROBIT_MIC_FAILED;

    return MICROBIT_OK;
}

int AES128::encrypt(AES_Encrypt* src, AES_Encrypt* destination)
{
    return encryptDecrypt(src, destination, true);
}

int AES128::decrypt(AES_Encrypt* src, AES_Encrypt* destination)
{
    return encryptDecrypt(src, destination, false);
}