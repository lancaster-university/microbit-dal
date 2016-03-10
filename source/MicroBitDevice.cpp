/**
  * Compatibility / portability funcitons and constants for the MicroBit DAL.
  */
#include "mbed.h"
#include "ErrorNo.h"

/**
  * Turn off warnings under gcc -Wall. We turn off unused-function for the
  * entire compilation unit as the compiler can't tell if a function is
  * unused until the end of the unit.  The macro expansion for SVCALL()
  * in nrf_soc.h and nrf_srv.h tries to leave unused-function turned off,
  * but we restore the state from before the include with our diagnostics
  * pop.
  * It might be cleaner to add
  * #pragram GCC system header
  * as the first line of nrf_soc.h, but that's a different
  * module ...
  */


/* 
 * The underlying Nordic libraries that support BLE do not compile cleanly with the stringent GCC settings we employ
 * If we're compiling under GCC, then we suppress any warnings generated from this code (but not the rest of the DAL)
 * The ARM cc compiler is more tolerant. We don't test __GNUC__ here to detect GCC as ARMCC also typically sets this
 * as a compatability option, but does not support the options used...
 */
#if !defined(__arm)
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "nrf_soc.h"
#include "nrf_sdm.h"

/* 
 * Return to our predefined compiler settings.
 */
#if !defined(__arm)
#pragma GCC diagnostic pop
#endif

/**
  * Determines if a BLE stack is currnetly running.
  * @return true is a bluetooth stack is operational, false otherwise.
  */
bool ble_running()
{
    uint8_t t;
    sd_softdevice_is_enabled(&t);
    return t==1;
}

