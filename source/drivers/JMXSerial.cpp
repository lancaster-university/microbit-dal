#include "JMXSerial.h"
#include "jmx.h"
#include "jmx_actions.h"
#include "MicroBitEvent.h"
#include "NotifyEvents.h"
#include "ErrorNo.h"
#include "MicroBitCompat.h"
#include "MicroBitFiber.h"
#include "EventModel.h"
#include "MicroBitConfig.h"

#define TOTAL_BUFFER_SIZE       36
#define EFFECTIVE_BUFF_SIZE     32

#define FILE_CLOSE_TIMEOUT           250

static ManagedBuffer buffer;

static const StatusPacket status_p_success = { 1 };
static const StatusPacket status_p_repeat =  { 0 };
static const StatusPacket status_p_error =   { -1 };

#ifdef __cplusplus
extern "C" {
#endif

void fs_request(void* data)
{
    buffer = ManagedBuffer((uint8_t *)data, sizeof(FSRequestPacket));
    MicroBitEvent(MICROBIT_ID_JMX, MICROBIT_JMX_EVT_FS);
}

void dir_request(void* data)
{
    buffer = ManagedBuffer((uint8_t*)data, sizeof(DIRRequestPacket));
    MicroBitEvent(MICROBIT_ID_JMX, MICROBIT_JMX_EVT_DIR);
}

void jmx_packet_received(char*) {}

void user_putc(char) {}

#ifdef __cplusplus
}
#endif

/**
  * Internal method to send a JSON formatted string.
  */
void JMXSerial::sendString(char* string)
{
    putc('"');
    for (uint8_t i = 0; i < strlen(string); i++)
        putc(string[i]);
    putc('"');
}

/**
  * Opens and configures the file descriptor for this JMXSerial instance.
  * It also adds this instance to idle components, so that the fd is reset
  * after a file has not been accessed for a short amount of time.
  *
  * @param fs A pointer to an instance of MicroBitFileSystem
  *
  * @param file the name of the file to open
  *
  * @param mode the mode to open the file in
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER on failure.
  */
int JMXSerial::openFile(const char* file, int mode)
{
    if(fd >= 0 && strcmp(filename.toCharArray(), file) != 0)
    {
        MicroBitFileSystem::defaultFileSystem->close(fd);
        filename = ManagedString();
        fd = -1;
    }

    if(fd < 0)
    {
        fd = MicroBitFileSystem::defaultFileSystem->open(file, mode);

        if(fd < 0)
            return MICROBIT_INVALID_PARAMETER;

        filename = file;

        if(status == 0)
            fiber_add_idle_component(this);

        status = FILE_CLOSE_TIMEOUT;
    }

    return MICROBIT_OK;
}

/**
  * Handles a read request from the interface chip.
  *
  * @param p The file system packet outlining the request
  */
void JMXSerial::fsRead(FSRequestPacket* p)
{
    openFile(p->filename, MB_READ);

    if(fd >= 0)
    {
        send("status", (void*)&status_p_success);

        MicroBitFileSystem::defaultFileSystem->seek(fd, p->offset, MB_SEEK_SET);

        char c = 0;
        int i = 0;

        while(MicroBitFileSystem::defaultFileSystem->read(fd, (uint8_t*)&c, 1) && i < p->len)
        {
            status = FILE_CLOSE_TIMEOUT;
            putc(c);
            i++;
        }

        c = 0;

        while(i < p->len)
        {
            putc(c);
            i++;
        }
    }
    else
        send("status", (void*)&status_p_error);
}

/**
  * Handles a write request from the interface chip.
  *
  * @param p The file system packet outlining the request
  */
void JMXSerial::fsWrite(FSRequestPacket* p)
{
    int i = 0;

    uint8_t buf[TOTAL_BUFFER_SIZE];

    uint32_t checksum = 0;
    uint32_t *rx_checksum = (uint32_t*)&buf[EFFECTIVE_BUFF_SIZE];
    int rx_size = 0;

    if(openFile(p->filename, MB_WRITE | MB_CREAT) != MICROBIT_OK)
    {
        putc(SLIP_ESC);
        putc('C');
        return;
    }

    MicroBitFileSystem::defaultFileSystem->seek(fd, p->offset, MB_SEEK_SET);

    putc(SLIP_ESC);
    putc('A');

    while(i < p->len)
    {
        status = FILE_CLOSE_TIMEOUT;

        rx_size = min(EFFECTIVE_BUFF_SIZE, p->len - i);

        read(buf, TOTAL_BUFFER_SIZE, SYNC_SPINWAIT);

        checksum = 0;

        for(int j = 0; j < EFFECTIVE_BUFF_SIZE; j++)
            checksum += buf[j];

        if (checksum != *rx_checksum)
        {
            putc(SLIP_ESC);
            putc('R');
            continue;
        }

        if(p->len - i > EFFECTIVE_BUFF_SIZE)
        {
            putc(SLIP_ESC);
            putc('A');
        }

        MicroBitFileSystem::defaultFileSystem->write(fd, buf, rx_size);

        i += rx_size;//
    }

    putc(SLIP_ESC);
    putc('A');
}

/**
  * Handles a deletion request from the interface chip.
  *
  * @param p The file system packet outlining the request
  */
void JMXSerial::fsDelete(FSRequestPacket* p)
{
    int localFd = MicroBitFileSystem::defaultFileSystem->remove(p->filename);

    if(localFd)
        send("status", (void*)&status_p_success);
    else
        send("status", (void*)&status_p_error);
}

/**
  * Handles a directory listing request.
  *
  * @param dir The directory request packet indicating the entry to return.
  */
void JMXSerial::fsList(DIRRequestPacket* dir)
{
    openFile("/", MB_READ);

    int entryOffset = 0;

    int entriesPerBlock = MBFS_BLOCK_SIZE / sizeof(DirectoryEntry);
    int excess = MBFS_BLOCK_SIZE - (sizeof(DirectoryEntry) * entriesPerBlock);

    int byteOffset = sizeof(DirectoryEntry);

    DirectoryEntry e;

    while(entryOffset < dir->entry + 1)
    {
        if(MicroBitFileSystem::defaultFileSystem->seek(fd, byteOffset, MB_SEEK_SET) < 0)
            break;

        if(MicroBitFileSystem::defaultFileSystem->read(fd, (uint8_t *)&e, sizeof(DirectoryEntry)) != sizeof(DirectoryEntry))
            break;

        if(e.flags & MBFS_DIRECTORY_ENTRY_VALID && !(e.flags & MBFS_DIRECTORY_ENTRY_DIRECTORY))
        {
            entryOffset++;

            // pad to the next block if necessary, we're offset by one due to the root entry
            if((entryOffset + 1) % entriesPerBlock == 0)
                byteOffset += excess;
        }

        byteOffset += sizeof(DirectoryEntry);
    }

    if(entryOffset == dir->entry + 1)
    {
        strncpy(dir->filename, e.file_name, 16);
        dir->size = e.length;
    }
    else
    {
        dir->entry = -1;
        dir->size = 0;
        *dir->filename = 0;
    }

    send("dir", (void *)dir);
}

/**
  * An event handler used to dispatch and process file system requests
  */
void JMXSerial::requestHandler(MicroBitEvent evt)
{
    if(evt.value == MICROBIT_JMX_EVT_FS)
    {
        FSRequestPacket* fsr = (FSRequestPacket*)&buffer[0];

        if (fsr->mode[0] == 'r')
            fsRead(fsr);

        if (fsr->mode[0] == 'w')
            fsWrite(fsr);

        if (fsr->mode[0] == 'd')
            fsDelete(fsr);
    }

    if(evt.value == MICROBIT_JMX_EVT_DIR)
    {
        DIRRequestPacket* dir = (DIRRequestPacket*)&buffer[0];
        fsList(dir);
    }

    buffer = ManagedBuffer();
}

/**
  * A callback which is used to close files after FILE_CLOSE_TIMEOUT number of
  * callbacks.
  */
void JMXSerial::idleTick()
{
    status--;

    if(status <= 1  && fd >= 0)
    {
        MicroBitEvent(7777,5);
        MicroBitFileSystem::defaultFileSystem->close(fd);
        fd = -1;

        fiber_remove_idle_component(this);
    }
}

/**
  * An internal interrupt callback for MicroBitSerial.
  *
  * Each time the Serial module's buffer is empty, write a character if we have
  * characters to write.
  */
void JMXSerial::dataWritten()
{
    if(txBuffTail == txBuffHead || !(serial_status & MICROBIT_SERIAL_TX_BUFF_INIT))
        return;

    int res = is_slip_character(txBuff[txBuffTail]);

    //send our current char
    if(res == ESC)
    {
        putc(SLIP_ESC);
        putc(SLIP_ESC_ESC);
    }
    else if(res == END)
    {
        putc(SLIP_ESC);
        putc(SLIP_ESC_END);
    }
    else
        putc(txBuff[txBuffTail]);

    uint16_t nextTail = (txBuffTail + 1) % txBuffSize;

    //unblock any waiting fibers that are waiting for transmission to finish.
    if(nextTail == txBuffHead)
    {
        MicroBitEvent(MICROBIT_ID_NOTIFY, MICROBIT_SERIAL_EVT_TX_EMPTY);
        detach(Serial::TxIrq);
    }

    //update our tail!
    txBuffTail = nextTail;
}

/**
  * An internal interrupt callback for MicroBitSerial configured for when a
  * character is received.
  *
  * Each time a character is received fill our circular buffer!
  */
void JMXSerial::dataReceived()
{
    //get the received character
    int chars[2] = {jmx_previous(), getc()};

    int jmx_ret = 0;

    //if the return token is false (0, -1), jmx has consumed the char...
    if((jmx_ret = jmx_state_track(chars[1])) <= 0)
        return;

    if(!(serial_status & MICROBIT_SERIAL_RX_BUFF_INIT))
        return;

    if(jmx_ret == 2 && is_slip_character(chars[0]) == ESC)
    {
        int slip = is_slip_character(chars[1]);

        if(slip == ESC_ESC)
            chars[1] = SLIP_ESC;

        if(slip == ESC_END)
            chars[1] = SLIP_END;

        jmx_ret = 1;
    }

    for(int i = 2 - jmx_ret; i < 2; i++)
    {
        char c = chars[i];

        int delimeterOffset = 0;
        int delimLength = this->delimeters.length();

        //iterate through our delimeters (if any) to see if there is a match
        while(delimeterOffset < delimLength)
        {
            //fire an event if there is to block any waiting fibers
            if(this->delimeters.charAt(delimeterOffset) == c)
                MicroBitEvent(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_DELIM_MATCH);

            delimeterOffset++;
        }

        uint16_t newHead = (rxBuffHead + 1) % rxBuffSize;

        //look ahead to our newHead value to see if we are about to collide with the tail
        if(newHead != rxBuffTail)
        {
            //if we are not, store the character, and update our actual head.
            this->rxBuff[rxBuffHead] = c;
            rxBuffHead = newHead;

            //if we have any fibers waiting for a specific number of characters, unblock them
            if(rxBuffHeadMatch >= 0)
                if(rxBuffHead == rxBuffHeadMatch)
                {
                    rxBuffHeadMatch = -1;
                    MicroBitEvent(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_HEAD_MATCH);
                }
        }
        else
        {
            //otherwise, our buffer is full, send an event to the user...
            MicroBitEvent(MICROBIT_ID_SERIAL, MICROBIT_SERIAL_EVT_RX_FULL);
            break;
        }
    }
}

/**
  * Constructor.
  * Create an instance of MicroBitSerial
  *
  * @param tx the Pin to be used for transmission
  *
  * @param rx the Pin to be used for receiving data
  *
  * @param rxBufferSize the size of the buffer to be used for receiving bytes
  *
  * @param txBufferSize the size of the buffer to be used for transmitting bytes
  *
  * @code
  * MicroBitSerial serial(USBTX, USBRX);
  * @endcode
  * @note the default baud rate is 115200. More API details can be found:
  *       -https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/api/SerialBase.h
  *       -https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/api/RawSerial.h
  *
  *       Buffers aren't allocated until the first send or receive respectively.
  */
JMXSerial::JMXSerial(PinName tx, PinName rx, uint8_t rxBufferSize, uint8_t txBufferSize) : MicroBitSerial(tx, rx, rxBufferSize, txBufferSize)
{
    jmx_init();

    fd = -1;
    status = 0;

    MicroBitFileSystem* fs = NULL;

    if(MicroBitFileSystem::defaultFileSystem == NULL)
        fs = new MicroBitFileSystem();
    else
        fs = MicroBitFileSystem::defaultFileSystem;

    if(fs && !(serial_status & MICROBIT_SERIAL_RX_BUFF_INIT))
    {
        // create and send our initialisation packet
        JMXInitPacket p;

        p.enable = 1;

#if CONFIG_ENABLED(MICROBIT_IF_CHIP_FS_OVERRIDE)
        // If we just want to preserve pages, we can send a packet with an explicit
        // disable.
        p.enable = 0;
#endif

        strcpy(p.v, MICROBIT_IF_CHIP_VERSION);

        p.p1 = 0;
        p.p2 = 0;

#if CONFIG_ENABLED(MICROBIT_IF_CHIP_PRESERVE)
        // tell the firmware to preserve these pages if the config option is enabled.
        p.p1 = MICROBIT_IF_PRES_1;
        p.p2 = MICROBIT_IF_PRES_2;
#endif
        send("jmx",(void*)&p);

        // we should only consume resources if we expect to respond to the interface
        // chip.
        if(p.enable)
        {
            serial_status |= MICROBIT_JMX_ENABLED;

            initialiseRx();

            if (EventModel::defaultEventBus)
                EventModel::defaultEventBus->listen(MICROBIT_ID_JMX, MICROBIT_EVT_ANY, this, &JMXSerial::requestHandler);
        }
    }
}

/**
  * Set a new baud rate for this Serial instance.
  *
  * @param baudrate the new baudrate. See:
  *         - https://github.com/mbedmicro/mbed/blob/master/libraries/mbed/targets/hal/TARGET_NORDIC/TARGET_MCU_NRF51822/serial_api.c
  *        for permitted baud rates.
  *
  * @return MICROBIT_INVALID_PARAMETER if baud rate is less than 0, otherwise MICROBIT_OK.
  *
  * @note the underlying implementation chooses the first allowable rate at or above that requested.
  */
void JMXSerial::baud(int baudrate)
{
    if(baudrate < 0)
        return;

    // we only are required to tell the Interface chip if jmx is enabled.
    if(serial_status & MICROBIT_JMX_ENABLED)
    {
        UARTConfigPacket p;

        p.baud = baudrate;

        send("uart", (void*)&p);
    }

    MicroBitSerial::baud(baudrate);
}

/**
  * Transmit a JMX packet over serial.
  *
  * @param identifier The identifier to look up in the action table.
  *
  * @param data The struct to send, which matches the struct used in the action table.
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if the identifier could not be found,
  *         or data is NULL.
  */
int JMXSerial::send(const char* identifier, void* data)
{
    lockTx();

    int table_index = -1;

    for (uint8_t i = 0; i < JMX_TABLE_COUNT; i++)
    {
        JMXActionTable* t = (JMXActionTable*)actionStore.actionTable[i];

        //if our key matches an item in the action table.
        if (strncmp(t->packet_identifier, (char *)identifier, min(strlen(t->packet_identifier), strlen(identifier))) == 0)
            table_index = i;
    }

    if (table_index == -1 || data == NULL)
      return MICROBIT_INVALID_PARAMETER;

    JMXActionTable* t = (JMXActionTable*)actionStore.actionTable[table_index];

    putc((char)JMX_ESCAPE_CHAR);

    putc((char)OBJECT_START);

    sendString((char *)t->packet_identifier);

    putc((char)FIELD_SEPARATOR_TOKEN);
    putc((char)OBJECT_START);

    for (uint8_t i = 0; i < JMX_ACTION_COUNT; i++)
    {
        JMXActionItem* a = &t->actions[i];
        if (a->type > 0 && a->key != NULL)
        {
            uint8_t* content = (uint8_t *)data + a->offset;

            //check if we have data...
            if (a->type == T_STATE_DYNAMIC_STRING && *((char **)content) == NULL)
                continue;

            //add a comma if applicable!
            if (i > 0)
                putc((char)PAIR_SEPARATOR_TOKEN);

            sendString((char *)a->key);

            putc((char)FIELD_SEPARATOR_TOKEN);

            if (a->type == T_STATE_STRING)
                sendString((char*)content);
            else if(a->type == T_STATE_DYNAMIC_STRING)
                sendString(*((char **)content));
            else if (a->type == T_STATE_NUMBER)
            {
                char result[MAX_JSON_NUMBER];
                int number;
                memcpy(&number, content, sizeof(int));
                itoa(number, result);

                for (uint8_t j = 0; j < strlen(result); j++)
                    putc(result[j]);
            }
        }
    }

    putc((char)OBJECT_END);
    putc((char)OBJECT_END);

    putc((char)JMX_END_CHAR);

    unlockTx();

    return MICROBIT_OK;
}

/**
  * Transmit a JMX packet over serial.
  *
  * @param identifier The identifier to look up in the action table.
  *
  * @param data The struct to send, which matches the struct used in the action table.
  *
  * @return MICROBIT_OK on success, MICROBIT_INVALID_PARAMETER if the identifier could not be found,
  *         or data is NULL.
  */
int JMXSerial::send(ManagedString identifier, void* data)
{
    return send(identifier.toCharArray(), data);
}
