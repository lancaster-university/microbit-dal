#ifndef JMX_PACKETS_H
#define JMX_PACKETS_H

#include <stdint.h>
#define NULL_TERMINATOR                1

typedef struct FSRequestPacket_t
{
    char filename[16 + NULL_TERMINATOR];
    int offset;
    int len;
    char mode[4 + NULL_TERMINATOR];
    char format[4 + NULL_TERMINATOR];
    char* base64;
} FSRequestPacket;

typedef struct DIRRequestPacket_t
{
    char filename[16 + NULL_TERMINATOR];
    int entry;
    int size;
} DIRRequestPacket;

typedef struct StatusPacket_t
{
    int code;
} StatusPacket;

typedef struct UARTConfigPacket_t
{
    int baud;
} UARTConfigPacket;

typedef struct JMXInitPacket_t
{
    int enable;
    char v[8 + NULL_TERMINATOR];
    int p1;
    int p2;
} JMXInitPacket;

typedef struct RedirectPacket_t
{
    char src[12 + NULL_TERMINATOR];
    char dest[12 + NULL_TERMINATOR];
} RedirectPacket;

#endif
