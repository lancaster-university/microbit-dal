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
    "MicroBitEventService.cpp"
    "MicroBitEvent.cpp"
    "MicroBitFiber.cpp"
    "ManagedString.cpp"
    "MicroBitAccelerometer.cpp"
    "MicroBitDFUService.cpp"
    "MicroBitIO.cpp"
    "MicroBitCompat.cpp"
    "MicroBitImage.cpp"
    "MicroBitDisplay.cpp"
    "DynamicPwm.cpp"
    "MicroBitPin.cpp"
    "MicroBitSerial.cpp"
)

if(CMAKE_COMPILER_IS_GNUCC)
  file(REMOVE "CortexContextSwitch.s")
  configure_file("CortexContextSwitch.s.gcc" "CortexContextSwitch.s" COPYONLY)
endif()

set(YOTTA_AUTO_MICROBIT-DAL_S_FILES
    "CortexContextSwitch.s"
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
