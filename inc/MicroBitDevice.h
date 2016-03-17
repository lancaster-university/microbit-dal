/**
 * Device specific funcitons for the nrf51822 device.
 *
 * Provides a degree of platform independence for microbit-dal functionality.
 * TODO: Determine is any o this belongs in an mbed target definition.
 * TODO: Review microbit-dal to place all such functions here.
 *
 */
#ifndef MICROBIT_DEVICE_H
#define MICROBIT_DEVICE_H
/*
 * The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ
 * If we're compiling under GCC, then we suppress any warnings generated from this code (but not the rest of the DAL)
 * The ARM cc compiler is more tolerant. We don't test __GNUC__ here to detect GCC as ARMCC also typically sets this
 * as a compatability option, but does not support the options used...
 */

/**
  * Determines if a BLE stack is currnetly running.
  * @return true is a bluetooth stack is operational, false otherwise.
  */
bool ble_running();

#endif
