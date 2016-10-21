#include "MicroBitSerial.h"
#include "ManagedString.h"
#include "ManagedBuffer.h"
#include "MicroBitComponent.h"
#include "MicroBitEvent.h"
#include "MicroBitFileSystem.h"

#include "jmx_packets.h"

#define MICROBIT_JMX_EVT_FS             1
#define MICROBIT_JMX_EVT_DIR            2

#define MICROBIT_JMX_DEFAULT_RX_SIZE    48


/**
  * JMXSerial is a handler for a Serial muxing protocol used for communicating
  * with the interface chip.
  *
  * JMX stands for JSON mux and uses SLIP (Serial Line Internet Protocol)
  * characters to delimit json packets.
  *
  * Communication occurs across the Serial line directly to the interface chip upon
  * filesystem interactions. This class handles these file system interactions using
  * events and transfers the data across the serial line.
  */
class JMXSerial : public MicroBitSerial, public MicroBitComponent
{
    // the currently open file name.
    ManagedString filename;

    // the currently open file descriptor returned by MicroBitFileSystem.
    int fd;

    /**
      * Internal method to send a JSON formatted string.
      */
    void sendString(char* string);

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
    int openFile(const char* file, int mode);

    /**
      * Handles a read request from the interface chip.
      *
      * @param p The file system packet outlining the request
      */
    void fsRead(FSRequestPacket* p);

    /**
      * Handles a write request from the interface chip.
      *
      * @param p The file system packet outlining the request
      */
    void fsWrite(FSRequestPacket* p);

    /**
      * Handles a deletion request from the interface chip.
      *
      * @param p The file system packet outlining the request
      */
    void fsDelete(FSRequestPacket* p);

    /**
      * Handles a directory listing request.
      *
      * @param dir The directory request packet indicating the entry to return.
      */
    void fsList(DIRRequestPacket* dir);

    /**
      * An event handler used to dispatch and process file system requests
      */
    void requestHandler(MicroBitEvent evt);

    /**
      * A callback which is used to close files after FILE_CLOSE_TIMEOUT number of
      * callbacks.
      */
    void idleTick();

    /**
      * An internal interrupt callback for MicroBitSerial configured for when a
      * character is received.
      *
      * Each time a character is received fill our circular buffer!
      */
    void dataReceived();

    /**
      * An internal interrupt callback for MicroBitSerial.
      *
      * Each time the Serial module's buffer is empty, write a character if we have
      * characters to write.
      */
    void dataWritten();

    public:

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
    JMXSerial(PinName tx, PinName rx, uint8_t rxBufferSize = MICROBIT_JMX_DEFAULT_RX_SIZE, uint8_t txBufferSize = MICROBIT_SERIAL_DEFAULT_BUFFER_SIZE);

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
    void baud(int baudrate);

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
    int send(const char* identifier, void* data);

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
    int send(ManagedString identifier, void* data);
};
