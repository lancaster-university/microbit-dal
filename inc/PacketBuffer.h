#ifndef MICROBIT_PACKET_BUFFER_H
#define MICROBIT_PACKET_BUFFER_H

#include "mbed.h"
#include "RefCounted.h"

struct PacketData : RefCounted
{
    uint16_t        rssi;               // The radio signal strength this packet was received.
    uint8_t         length;             // The length of the payload in bytes
    uint8_t         payload[0];         // User / higher layer protocol data
};

/**
  * Class definition for a PacketBuffer.
  * A PacketBuffer holds a series of bytes that can be sent or received from the MicroBitRadio channel.
  * n.b. This is a mutable, managed type.
  */
class PacketBuffer
{
    PacketData      *ptr;     // Pointer to payload data
    
    public:

    /**
      * Provide an array containing the packet data.
      * @return The contents of this packet, as an array of bytes.
      */
    uint8_t *getBytes();

    /**
      * Default Constructor. 
      * Creates an empty Packet Buffer. 
      *
      * Example:
      * @code
      * PacketBuffer p(); 
      * @endcode
      */
    PacketBuffer();

    /**
      * Constructor. 
      * Creates a new PacketBuffer of the given size. 
      *
      * @param length The length of the buffer to create.
      *
      * Example:
      * @code
      * PacketBuffer p(16);         // Creates a PacketBuffer 16 bytes long.
      * @endcode
      */
    PacketBuffer(int length);

    /**
      * Constructor. 
      * Creates an empty Packet Buffer of the given size,
      * and fills it with the data provided.
      *
      * @param data The data with which to fill the buffer.
      * @param length The length of the buffer to create.
      * @param rssi The radio signal strength at the time this pacer was recieved. 
      * 
      * Example:
      * @code
      * uint8_t buf = {13,5,2};
      * PacketBuffer p(buf, 3);         // Creates a PacketBuffer 3 bytes long.
      * @endcode
      */
    PacketBuffer(uint8_t *data, int length, int rssi = 0);

    /**
      * Copy Constructor. 
      * Add ourselves as a reference to an existing PacketBuffer.
      * 
      * @param buffer The PacketBuffer to reference.
      *
      * Example:
      * @code
      * PacketBuffer p();
      * PacketBuffer p2(i);        // Refers to the same packet as p. 
      * @endcode
      */
    PacketBuffer(const PacketBuffer &buffer);

    /**
     * Internal constructor-initialiser.
     *
     * @param data The data with which to fill the buffer.
     * @param length The length of the buffer to create.
     * @param rssi The radio signal strength at the time this packet was recieved. 
     * 
     */
    void init(uint8_t *data, int length, int rssi);

    /**
      * Destructor. 
      * Removes buffer resources held by the instance.
      */
    ~PacketBuffer();

    /**
      * Copy assign operation. 
      *
      * Called when one PacketBuffer is assigned the value of another using the '=' operator.
      * Decrements our reference count and free up the buffer as necessary.
      * Then, update our buffer to refer to that of the supplied PacketBuffer,
      * and increase its reference count.
      *
      * @param p The PacketBuffer to reference.
      * 
      * Example:
      * @code
      * uint8_t buf = {13,5,2};
      * PacketBuffer p1(16); 
      * PacketBuffer p2(buf, 3);        
      *
      * p1 = p2;  
      * @endcode
      */
    PacketBuffer& operator = (const PacketBuffer& p);

    /**
     * Array access operation (read). 
     *
     * Called when a PacketBuffer is dereferenced with a [] operation.
     * Transparently map this through to the underlying payload for elegance of programming.
     *
     * Example:
     * @code
     * PacketBuffer p1(16); 
     * uint8_t data = p1[0];
     * @endcode
     */
    uint8_t operator [] (int i) const;

    /**
     * Array access operation (modify). 
     *
     * Called when a PacketBuffer is dereferenced with a [] operation.
     * Transparently map this through to the underlying payload for elegance of programming.
     *
     * Example:
     * @code
     * PacketBuffer p1(16); 
     * p1[0] = 42;
     * @endcode
     */
    uint8_t& operator [] (int i);

    /**
      * Equality operation.
      *
      * Called when one PacketBuffer is tested to be equal to another using the '==' operator.
      *
      * @param p The PacketBuffer to test ourselves against.
      * @return true if this PacketBuffer is identical to the one supplied, false otherwise.
      * 
      * Example:
      * @code
      *
      * uint8_t buf = {13,5,2};
      * PacketBuffer p1(16); 
      * PacketBuffer p2(buf, 3);        
      *
      * if(p1 == p2)                    // will be true
      *     uBit.display.scroll("same!");
      * @endcode
      */
    bool operator== (const PacketBuffer& p);
    
    /**
      * Sets the byte at the given index to value provided.
      * @param position The index of the byte to change.
      * @param value The new value of the byte (0-255).
      * @return MICROBIT_OK, or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * PacketBuffer p1(16); 
      * p1.setByte(0,255);              // Sets the first byte in the buffer to the value 255.
      * @endcode
      */
    int setByte(int position, uint8_t value);

    /**
      * Determines the value of the given byte in the packet.
      *
      * @param position The index of the byte to read.
      * @return The value of the byte at the given position, or MICROBIT_INVALID_PARAMETER.
      *
      * Example:
      * @code
      * PacketBuffer p1(16); 
      * p1.setByte(0,255);              // Sets the first byte in the buffer to the value 255.
      * p1.getByte(0);                  // Returns 255.
      * @endcode
      */
    int getByte(int position);

    /**
      * Gets number of bytes in this buffer 
      * @return The size of the buffer in bytes.
      * 
      * Example:
      * @code
      * PacketBuffer p1(16); 
      * p1.length();                 // Returns 16.
      * @endcode
      */
    int length();

    /**
      * Gets the received signal strength of this packet.
      *
      * @return The signal strength of the radio when this packet was received, in -dbM.
      * 
      * Example:
      * @code
      * PacketBuffer p1(16); 
      * p1.getRSSI();                 // Returns the received signal strength.
      * @endcode
      */
    int getRSSI();

    /**
     * Sets the received signal strength of this packet.
     *
     * Example:
     * @code
     * PacketBuffer p1(16); 
     * p1.setRSSI(37);                 
     * @endcode
     */
    void setRSSI(uint8_t rssi);

    static PacketBuffer EmptyPacket;
};

#endif

