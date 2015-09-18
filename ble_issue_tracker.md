# micro:bit BLE - Issue Tracker
----------

Profile Design
--------------

### OPEN:

D3. Simplify the IO Pin Service, possible to expose the edge connector pins only. Possibly drop it to save memory and use the event service to transport pin values in either direction. Needs further thought.

* Update: Removed all pin characteristics except for pin 0, pin 1 and pin 2.

D5. Generic Attribute Service: profile design doc/report doesn’t show it and it’s mandatory (issue with Bluetooth Developer Studio). 

D9. DFU services uses a different base UUID to the other custom services. Is this deliberate?

D10. Characteristics in the DFU service use a different base UUID to the parent service. Is this deliberate?

D11. What are the data types for DFU Control and DFU Flash Code? Assumed uint8 and array of uint8.


### CLOSED:

D1. Lose the System LED State characteristic since it cannot be controlled from the BLE MCU.DONE.

D2. Lose the Scrolling State characteristic – complexity and memory constraints. DONE.

D4. Generic Access Service: Peripheral Privacy Flag is optional and I don’t think we need it. Ditto Reconnection Address, ditto Peripheral Preferred Connection Parameters --> Removed optional characteristics Peripheral Privacy Flag, Reconnection Address and Peripheral Preferred Connection Parameters from Generic Access Service.

D6. Device Information Service: All characteristics are optional. Which ones do we really want/need? Save a little memory --> Removed PnP ID, IEEE 11073-20601 Regulatory Certification Data List, System ID and Software Revision String characteristics.

D7. Why does LED Matrix State support “Write Without Response”? I think this should be plain “Write” so that no further writes are made until there’s been an ACK back from the previous one. Changed.

D8. MicroBit Requirements supports Write. This is (ironically and puntastically) wrong. Client should not be able to modify the requirements the MicroBit has. Changed.

D12. Microbit DFU service not in the profile design. Added. Needs descriptions and data types confirming.


Profile Testing
---------------

### OPEN:
T1. Client Event characteristic should have the WRITE property and currently does not.

T2. MicroBit Event characteristic should have the READ property and currently does not.

T3. MicroBit Event characteristic should have the NOTIFY property and currently does not.

T4. MicroBit Requirements characteristic is missing from the Event Service

T5. Client Requirements characteristic is missing from the Event Service

T6. Device Name in advertising packets includes the flash code so anyone could pair to it. Should be removed.

T7. Generic Access Service: Device Name and Appearance are mandatory and so need values

T8. Review advertising parameters: Advertising frequency seems quite low. This will make the discovery process slower and overall, both the pairing and FOTA processes a little slower. Are all three advertising channels in use or not? Thinking of a class room or event it would be best to use all three.  

T9. Consider using directed advertising and white listing so that only the paired peer device can (re)connect to the micro:bit. I believe we are currently performing undirected advertising.... basically broadcasting. Directed advertising will address ADV packets to the paired peer device only. As things stand it seems to me that if some other person in the classroom just decides to connect to a micro:bit that is not theirs, they’ve effectively blocked all other use of the device.... a simple DOS attack.

### CLOSED:
