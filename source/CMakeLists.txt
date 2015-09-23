# This file is no longer auto-generated to make the repository builds with GCC
# and ARMCC no matter what.

cmake_minimum_required(VERSION 2.8.11)

enable_language(ASM)

set(YOTTA_AUTO_MICROBIT-DAL_CPP_FILES
    "MicroBitSuperMain.cpp"
    "MicroBitI2C.cpp"
    "MicroBitMultiButton.cpp"
    "MicroBitFont.cpp"
    "MicroBit.cpp"
    "MicroBitButton.cpp"
    "MicroBitMessageBus.cpp"
    "MicroBitCompass.cpp"
    "MicroBitEvent.cpp"
    "MicroBitFiber.cpp"
    "ManagedString.cpp"
    "MicroBitAccelerometer.cpp"
    "MicroBitIO.cpp"
    "MicroBitCompat.cpp"
    "MicroBitImage.cpp"
    "MicroBitDisplay.cpp"
    "DynamicPwm.cpp"
    "MicroBitPin.cpp"
    "MicroBitSerial.cpp"
    "MicroBitHeapAllocator.cpp"
    "MicroBitListener.cpp"
    "MemberFunctionCallback.cpp"
    "ble-services/MicroBitDFUService.cpp"
    "ble-services/MicroBitEventService.cpp"
    "ble-services/MicroBitLEDService.cpp"
    "ble-services/MicroBitAccelerometerService.cpp"
    "ble-services/MicroBitMagnetometerService.cpp"
    "ble-services/MicroBitButtonService.cpp"
    "ble-services/MicroBitIOPinService.cpp"
    "ble-services/MicroBitTemperatureService.cpp"
)

if (YOTTA_CFG_MICROBIT_CONFIGFILE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${YOTTA_FORCE_INCLUDE_FLAG} \"${YOTTA_CFG_MICROBIT_CONFIGFILE}\"")
endif ()

if(CMAKE_COMPILER_IS_GNUCC)
  file(REMOVE "asm/CortexContextSwitch.s")
  configure_file("asm/CortexContextSwitch.s.gcc" "asm/CortexContextSwitch.s" COPYONLY)
else()
  file(REMOVE "asm/CortexContextSwitch.s")
  configure_file("asm/CortexContextSwitch.s.armcc" "asm/CortexContextSwitch.s" COPYONLY)
endif()

set(YOTTA_AUTO_MICROBIT-DAL_S_FILES
    "asm/CortexContextSwitch.s"
)

add_library(microbit-dal
    ${YOTTA_AUTO_MICROBIT-DAL_CPP_FILES}
    ${YOTTA_AUTO_MICROBIT-DAL_S_FILES}
)

yotta_postprocess_target(LIBRARY microbit-dal)

target_link_libraries(microbit-dal
    mbed-classic
    ble
    ble-nrf51822
)
