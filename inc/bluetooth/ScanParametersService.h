#ifndef __BLE_SCAN_PARAMETERS_SERVICE_H__
#define __BLE_SCAN_PARAMETERS_SERVICE_H__

#include "ble/BLE.h"

#pragma pack(push, 1)
typedef struct {
	uint16_t LE_Scan_Interval;
	uint16_t LE_Scan_Window;
} ScanIntervalWindow_t;
#pragma pack(pop)

class ScanParametersService {
public:
	ScanParametersService(
		BLE &_ble
	) :
		ble(_ble),
		scanIntervalWindowCharacteristic(
			GattCharacteristic::UUID_SCAN_INTERVAL_WINDOW_CHAR,
			(uint8_t*)&scanIntervalWindowData,
			sizeof(scanIntervalWindowData),
			sizeof(scanIntervalWindowData),
			GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE_WITHOUT_RESPONSE
		),
		scanRefreshCharacteristic(
			GattCharacteristic::UUID_SCAN_REFRESH_CHAR,
			&scanRefreshData,
			sizeof(scanIntervalWindowData),
			sizeof(scanIntervalWindowData),
			GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY
		)
	{
		GattCharacteristic *charTable[] = {
			&scanIntervalWindowCharacteristic,
			&scanRefreshCharacteristic
		};

		GattService scanParametersService(
			GattService::UUID_SCAN_PARAMETERS_SERVICE,
			charTable,
			sizeof(charTable) / sizeof(GattCharacteristic *)
		);

		ble.addService(scanParametersService);
	}

protected:
	BLE& ble;

	ScanIntervalWindow_t scanIntervalWindowData;
	uint8_t scanRefreshData;

	GattCharacteristic scanIntervalWindowCharacteristic;
	GattCharacteristic scanRefreshCharacteristic;
};

#endif
