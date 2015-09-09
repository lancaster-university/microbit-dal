# micro:bit BLE profile : Tracking changes from planned specification

This document briefly outlines changes to the planned BLE specification made during implementation.
These are typically made with purpose, so the rationale is also provided where possible.

## LEDService:
* Lose the System LED State characteristic since it cannot be controlled from the BLE MCU.
* Lose the Scrolling State characteristic due to complexity and memory constraints.

## IO Pin Service:
* Simplify the IO Pin Service, possible to expose the edge connector pins only. 
* Possibly drop it to save memory and use the event service to transport pin values in either direction. Needs further thought.

## Generic Access Service: 
* Device Name and Appearance are mandatory and so need values
* Peripheral Privacy Flag is optional and I don’t think we need it. 
* Ditto Reconnection Address.
* Ditto Peripheral Preferred Connection Parameters.

## Generic Attribute Service: 
* profile design doc/report doesn’t show it and it’s mandatory (issue with Bluetooth Developer Studio). 

## Device Information Service: 
* All characteristics are optional. Which ones do we really want/need? Save a little memory.....


