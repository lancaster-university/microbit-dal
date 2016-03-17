#ifndef MICROBIT_TEMPERATURE_SERVICE_H
#define MICROBIT_TEMPERATURE_SERVICE_H

#include "ble/BLE.h"
#include "MicroBitThermometer.h"
#include "EventModel.h"

// UUIDs for our service and characteristics
extern const uint8_t  MicroBitTemperatureServiceUUID[];
extern const uint8_t  MicroBitTemperatureServiceDataUUID[];
extern const uint8_t  MicroBitTemperatureServicePeriodUUID[];


/**
  * Class definition for a MicroBit BLE Temperture Service.
  * Provides access to live temperature data via BLE.
  */
class MicroBitTemperatureService
{
    public:

    /**
      * Constructor.
      * Create a representation of the TempertureService
      * @param _ble The instance of a BLE device that we're running on.
      */
    MicroBitTemperatureService(BLEDevice &_ble, MicroBitThermometer &_thermometer, EventModel &messageBus);

    /**
      * Callback. Invoked when any of our attributes are written via BLE.
      */
    void onDataWritten(const GattWriteCallbackParams *params);

    /**
     * Temperature update callback
     */
    void temperatureUpdate(MicroBitEvent e);

    private:

    // Bluetooth stack we're running on.
    BLEDevice           	&ble;
    MicroBitThermometer     &thermometer;

    // memory for our 8 bit temperature characteristic.
    int8_t             temperatureDataCharacteristicBuffer;
    uint16_t           temperaturePeriodCharacteristicBuffer;

    // Handles to access each characteristic when they are held by Soft Device.
    GattAttribute::Handle_t temperatureDataCharacteristicHandle;
    GattAttribute::Handle_t temperaturePeriodCharacteristicHandle;
};


#endif
