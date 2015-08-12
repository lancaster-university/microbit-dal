#include "MicroBit.h"
#include "mbed.h"
#include "twi_master.h"
#include "nrf_delay.h"

/**
  * Constructor. 
  * Create an instance of i2c
  * @param sda the Pin to be used for SDA
  * @param scl the Pin to be used for SCL
  * Example:
  * @code 
  * MicroBitI2C i2c(MICROBIT_PIN_SDA, MICROBIT_PIN_SCL);
  * @endcode
  * @note this should prevent i2c lockups as well.
  */
MicroBitI2C::MicroBitI2C(PinName sda, PinName scl) : I2C(sda,scl)
{
    this->retries = 0;
}


int MicroBitI2C::read(int address, char *data, int length, bool repeated)
{
    int result = I2C::read(address,data,length,repeated);
    
    //0 indicates a success, presume failure
    while(result != 0 && retries < MICROBIT_I2C_MAX_RETRIES)
    {
        _i2c.i2c->EVENTS_ERROR = 0;
        _i2c.i2c->ENABLE       = TWI_ENABLE_ENABLE_Disabled << TWI_ENABLE_ENABLE_Pos; 
        _i2c.i2c->POWER        = 0;
        nrf_delay_us(5);
        _i2c.i2c->POWER        = 1;
        _i2c.i2c->ENABLE       = TWI_ENABLE_ENABLE_Enabled << TWI_ENABLE_ENABLE_Pos;
        twi_master_init_and_clear();
        result = I2C::read(address,data,length,repeated);
        retries++;
    }
    
    if(retries == MICROBIT_I2C_MAX_RETRIES - 1)
        uBit.panic(MICROBIT_I2C_LOCKUP);
    
    retries = 0;
    
    return result;
}

int MicroBitI2C::write(int address, const char *data, int length, bool repeated)
{   
    

    int result = I2C::write(address,data,length,repeated);
    
    //0 indicates a success, presume failure
    while(result != 0 && retries < MICROBIT_I2C_MAX_RETRIES)
    {
        _i2c.i2c->EVENTS_ERROR = 0;
        _i2c.i2c->ENABLE       = TWI_ENABLE_ENABLE_Disabled << TWI_ENABLE_ENABLE_Pos; 
        _i2c.i2c->POWER        = 0;
        nrf_delay_us(5);
        _i2c.i2c->POWER        = 1;
        _i2c.i2c->ENABLE       = TWI_ENABLE_ENABLE_Enabled << TWI_ENABLE_ENABLE_Pos;
        
        twi_master_init_and_clear();
        result = I2C::write(address,data,length,repeated);
        retries++;
    }
    
    if(retries == MICROBIT_I2C_MAX_RETRIES - 1)
        uBit.panic(MICROBIT_I2C_LOCKUP);
        
    retries = 0;
    
    return result;
}
