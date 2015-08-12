#ifndef MICROBIT_COMPONENT_H
#define MICROBIT_COMPONENT_H

/**
  * Class definition for MicroBitComponent
  * All components should inherit from this class.
  * @note if a component needs to be called regularly, then you should add the component to the systemTick and idleTick queues.
  * If it's in the systemTick queue, you should override systemTick and implement the required functionality.
  * Similarly if the component is in the idleTick queue, the idleTick member function should be overridden.
  */
class MicroBitComponent
{
    protected:
    
    uint16_t id;                    // Event Bus ID
    uint8_t status;                 // keeps track of various component state, and also indicates if data is ready.
    
    public:
    
    /**
      * The default constructor of a MicroBitComponent
      */
    MicroBitComponent()
    {
        this->id = 0;
        this->status = 0;
    }
    
    /**
      * Once added to the systemTickComponents array, this member function will be
      * called in interrupt context on every system tick. 
      */
    virtual void systemTick(){
        
    }

    /**
      * Once added to the idleThreadComponents array, this member function will be
      * called in idle thread context indiscriminately.
      */
    virtual void idleTick()
    {
    
    }
    
    /**
      * When added to the idleThreadComponents array, this function will be called to determine
      * if and when data is ready.
      * @note override this if you want to request to be scheduled imminently
      */
    virtual int isIdleCallbackNeeded()
    {
        return 0;   
    }
    
    virtual ~MicroBitComponent()
    {
    
    }
};

#endif
