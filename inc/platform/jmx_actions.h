#ifndef JMX_ACTIONS_H
#define JMX_ACTIONS_H

#include "jmx_packets.h"
#include "jmx.h"
#include <stdint.h>
#include <stdlib.h>

#define KEY_BUFFER_SIZE            10
#define JMX_ACTION_COUNT           6
#define JMX_TABLE_COUNT            5

#define member_size(type, member)    sizeof(((type *)0)->member)
//#define offsetof(st, m) ((size_t)&(((st *)0)->m))

typedef void (*JMXFunctionPointer)(void* buffer);

enum ActionTableType {
    STANDARD_ACTION,
    BUFFERED_ACTION
};

typedef struct JMXActionItem_t
{
    const char* key;
    uint8_t type;
    uint32_t offset;
    uint32_t buffer_size;
} JMXActionItem;

typedef struct JMXActionTable_t
{
    uint32_t packet_type;
    const char packet_identifier[KEY_BUFFER_SIZE];
    uint32_t struct_size;
    void** pointer_base;
    JMXFunctionPointer fp;
    JMXActionItem actions[JMX_ACTION_COUNT];
} JMXActionTable;

typedef struct JMXActionStore
{
    const JMXActionTable* actionTable[JMX_TABLE_COUNT];
} JMXActionStore;



#ifdef __cplusplus
extern "C" {
#endif

extern void fs_request(void* data);
extern void nop(void*);
extern void dir_request(void* data);
extern void uart_request(void* data);

extern JMXInitPacket* jmx_init_p;
extern FSRequestPacket* jmx_fsr_p;
extern DIRRequestPacket* jmx_dir_p;
extern StatusPacket* jmx_status_p;
extern RedirectPacket* jmx_redirect_p;
extern UARTConfigPacket* jmx_uart_p;

#ifdef __cplusplus
}
#endif

/**
  * A JMXActionTable provides a schema for a JSON string to be translated into a c style struct.
  *
  * For the JMXActionTable below, here's an example of what a valid json string would look like:
  *
  *    {
  *        "fs" : {
  *            "fname":"EXAMPLE.TXT",
  *            "offset": 0,
  *            "len": 100,
  *            "mode": "read",
  *            "format": "b64",
  *        }
  *    }
  *
  * As you can see, the JMXActionTable offers a translation to the FSRequestPacket struct. Fields can be given in any order, and the packet identifier
  *    offers a mechanism to determine the JMXActionTable to be used.
  */
static const JMXActionTable fsMap = {
    STANDARD_ACTION,
    "fs",                        //Packet identifier
    sizeof(FSRequestPacket),    // struct size
    (void **)&jmx_fsr_p,                // pointer base
    fs_request,                // result function pointer
    {
        //    KEY            TOKEN TYPE                STORAGE OFFSET INTO STRUCT                    SIZE OF STORAGE BUFFER
        {    "fname",    T_STATE_STRING,            offsetof(FSRequestPacket, filename),    member_size(FSRequestPacket, filename)    },
        {    "offset",    T_STATE_NUMBER,            offsetof(FSRequestPacket, offset),        member_size(FSRequestPacket, offset)    },
        {    "len",        T_STATE_NUMBER,            offsetof(FSRequestPacket, len),            member_size(FSRequestPacket, len)        },
        {    "mode",        T_STATE_STRING,            offsetof(FSRequestPacket, mode),        member_size(FSRequestPacket, mode)        },
        {    "format",    T_STATE_STRING,            offsetof(FSRequestPacket, format),        member_size(FSRequestPacket, format)    },
        {    "b64",        T_STATE_DYNAMIC_STRING,    offsetof(FSRequestPacket, base64),        2                                        }
    }
};

/**
*    {
*        "dir" : {
*            "entry": X,
*            "fname":"POO.TXT",
*            "size":XXX
*        }
*    }
*/
static const JMXActionTable dirMap = {
    STANDARD_ACTION,
    "dir",                        //Packet identifier
    sizeof(DIRRequestPacket),    // struct size
    (void **)&jmx_dir_p,                // pointer base
    dir_request,                // result function pointer
    {
        //    KEY            TOKEN TYPE                STORAGE OFFSET INTO STRUCT                    SIZE OF STORAGE BUFFER
        { "entry",        T_STATE_NUMBER,            offsetof(DIRRequestPacket, entry),            member_size(DIRRequestPacket, entry)    },
        { "fname",        T_STATE_STRING,            offsetof(DIRRequestPacket, filename),        member_size(DIRRequestPacket, filename) },
        { "size",        T_STATE_NUMBER,            offsetof(DIRRequestPacket, size),            member_size(DIRRequestPacket, size)        },
    }
};

/**
  *    {
  *        "jmx" : {
  *            "enable": 0/1,
  *            "v":"XX.XX.XX"
  *        }
  *    }
  */
static const JMXActionTable initMap = {
    STANDARD_ACTION,
    "jmx",                        //Packet identifier
    sizeof(JMXInitPacket),        // struct size
    (void **)&jmx_init_p,        // pointer base
    nop,                // result function pointer
    {
        //    KEY            TOKEN TYPE                STORAGE OFFSET INTO STRUCT                    SIZE OF STORAGE BUFFER
        { "enable",        T_STATE_NUMBER,            offsetof(JMXInitPacket, enable),            member_size(JMXInitPacket, enable)    },
        { "v",            T_STATE_STRING,            offsetof(JMXInitPacket, v),                    member_size(JMXInitPacket, v)        },
        { "p1",            T_STATE_NUMBER,            offsetof(JMXInitPacket, p1),                member_size(JMXInitPacket, p1)        },
        { "p2",            T_STATE_NUMBER,            offsetof(JMXInitPacket, p2),                member_size(JMXInitPacket, p2)        },
    }
};

/**
  *    {
  *        "redirect" : {
  *            "src": "interface",
  *            "dest": "interface"
  *        }
  *    }
  */
const JMXActionTable redirectMap = {
    STANDARD_ACTION,
    "redirect",
    sizeof(RedirectPacket),    // struct size
    (void **)&jmx_redirect_p,        // pointer base
    nop,            // result function pointer
    {
        //    KEY            TOKEN TYPE            STORAGE OFFSET INTO STRUCT        SIZE OF STORAGE BUFFER
        {    "src",        T_STATE_NUMBER,        offsetof(RedirectPacket, src),    member_size(RedirectPacket, src) },
        {    "dest",        T_STATE_NUMBER,        offsetof(RedirectPacket, dest),    member_size(RedirectPacket, dest) },
    }
};

/**
  *    {
  *        "status" : {
  *            "code": MICROBIT_ERROR_CODE
  *        }
  *    }
  */
const JMXActionTable statusMap = {
    STANDARD_ACTION,
    "status",
    sizeof(StatusPacket),    // struct size
    (void **)&jmx_status_p,        // pointer base
    nop,            // result function pointer
    {
        //    KEY            TOKEN TYPE            STORAGE OFFSET INTO STRUCT        SIZE OF STORAGE BUFFER
        { "code",        T_STATE_NUMBER,        offsetof(StatusPacket, code),    member_size(StatusPacket, code) },
    }
};

/**
*    {
*        "uart" : {
*            "baud": baud_rate
*        }
*    }
*/
const JMXActionTable uartMap = {
    STANDARD_ACTION,
    "uart",
    sizeof(UARTConfigPacket),    // struct size
    (void **)&jmx_uart_p,        // pointer base
    nop,                        // result function pointer
    {
        //    KEY            TOKEN TYPE            STORAGE OFFSET INTO STRUCT        SIZE OF STORAGE BUFFER
        { "baud",        T_STATE_NUMBER,        offsetof(UARTConfigPacket, baud),    member_size(UARTConfigPacket, baud) },
    }
};

const JMXActionStore actionStore =
{
    &fsMap,
    &dirMap,
    &initMap,
    &statusMap,
    &uartMap
};
#endif
