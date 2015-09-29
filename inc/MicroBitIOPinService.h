#ifndef MICROBIT_IO_PIN_SERVICE_H
#define MICROBIT_IO_PIN_SERVICE_H

#include "MicroBit.h"

#define MICROBIT_IO_PIN_SERVICE_PINCOUNT       20 
#define MICROBIT_IO_PIN_SERVICE_DATA_SIZE      10 

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitIOPinServiceUUID[];
extern const uint8_t  MicroBitIOPinServiceADConfigurationUUID[];
extern const uint8_t  MicroBitIOPinServiceIOConfigurationUUID[];
extern const uint8_t  MicroBitIOPinServiceDataUUID[];
extern MicroBitPin * const MicroBitIOPins[];

/**
  * Name value pair definition, as used to read abd write pin values over BLE.
  */
struct IOData
{
    uint8_t     pin;
    uint8_t     value;
};

/**
  * Class definition for a MicroBit BLE IOPin Service.
  * Provides access to live ioPin data via BLE, and provides basic configuration options.
  */
class MicroBitIOPinService : public MicroBitComponent
{                                    
    public:
    
    /**
      * Constructor. 
      * Create a representation of the IOPinService
      * @param _ble The instance of a BLE device that we're running on.
      */
    MicroBitIOPinService(BLEDevice &_ble);  

    /**
     * periodic callback from MicroBit scheduler.
     * Check if any of the pins we're watching need updating. Apply a BLE NOTIFY if so... 
     */  
    virtual void idleTick();    

    private:

    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    void onDataWritten(const GattWriteCallbackParams *params);

    /**
     * Callback. invoked when the BLE data characteristic is read.
     * reads all the pins marked as inputs, and updates the data stored in the BLE stack.
     */  
    void onDataRead(GattReadAuthCallbackParams *params);

    /**
      * Determines if the given pin was configured as a digital pin by the BLE ADPinConfigurationCharacterisitic.
      *
      * @param pin the enumeration of the pin to test
      * @return 1 if this pin is configured as a digital value, 0 otherwise
      */
    int isDigital(int i); 

    /**
      * Determines if the given pin was configured as an analog pin by the BLE ADPinConfigurationCharacterisitic.
      *
      * @param pin the enumeration of the pin to test
      * @return 1 if this pin is configured as a analog value, 0 otherwise
      */
    int isAnalog(int i); 

    /**
      * Determines if the given pin was configured as an input by the BLE IOPinConfigurationCharacterisitic.
      *
      * @param pin the enumeration of the pin to test
      * @return 1 if this pin is configured as an input, 0 otherwise
      */
    int isInput(int i); 

    /**
      * Determines if the given pin was configured as output by the BLE IOPinConfigurationCharacterisitic.
      *
      * @param pin the enumeration of the pin to test
      * @return 1 if this pin is configured as an output, 0 otherwise
      */
    int isOutput(int i); 


    // Bluetooth stack we're running on.
    BLEDevice           &ble;

    // memory for our 8 bit control characteristics.
    uint32_t            ioPinServiceADCharacteristicBuffer;
    uint32_t            ioPinServiceIOCharacteristicBuffer;
    IOData              ioPinServiceDataCharacteristicBuffer[MICROBIT_IO_PIN_SERVICE_DATA_SIZE];

    // Historic information about our pin data data.
    uint8_t             ioPinServiceIOData[MICROBIT_IO_PIN_SERVICE_PINCOUNT];

    // Handles to access each characteristic when they are held by Soft Device.
    GattAttribute::Handle_t ioPinServiceADCharacteristicHandle;
    GattAttribute::Handle_t ioPinServiceIOCharacteristicHandle;
    GattCharacteristic *ioPinServiceDataCharacteristic;
};


#endif

