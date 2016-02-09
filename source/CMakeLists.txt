# This file is no longer auto-generated to make the repository builds with GCC
# and ARMCC no matter what.

cmake_minimum_required(VERSION 2.8.12)

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
    "Matrix4.cpp"
    "MicroBitAccelerometer.cpp"
    "MicroBitThermometer.cpp"
    "MicroBitIO.cpp"
    "MicroBitCompat.cpp"
    "MicroBitImage.cpp"
    "MicroBitDisplay.cpp"
    "DynamicPwm.cpp"
    "MicroBitPin.cpp"
    "MicroBitSerial.cpp"
    "MicroBitHeapAllocator.cpp"
    "MicroBitListener.cpp"
    "MicroBitLightSensor.cpp"
    "RefCounted.cpp"
    "MemberFunctionCallback.cpp"
    "MicroBitStorage.cpp"
    "PacketBuffer.cpp"
    "ble-services/MicroBitBLEManager.cpp"
    "ble-services/MicroBitDFUService.cpp"
    "ble-services/MicroBitEventService.cpp"
    "ble-services/MicroBitLEDService.cpp"
    "ble-services/MicroBitAccelerometerService.cpp"
    "ble-services/MicroBitMagnetometerService.cpp"
    "ble-services/MicroBitButtonService.cpp"
    "ble-services/MicroBitIOPinService.cpp"
    "ble-services/MicroBitTemperatureService.cpp"
    "ble-services/MicroBitRadio.cpp"
    "ble-services/MicroBitRadioDatagram.cpp"
    "ble-services/MicroBitRadioEvent.cpp"
)

execute_process(WORKING_DIRECTORY "../../yotta_modules/${PROJECT_NAME}" COMMAND "git" "log" "--pretty=format:%h" "-n" "1" OUTPUT_VARIABLE git_hash)
execute_process(WORKING_DIRECTORY "../../yotta_modules/${PROJECT_NAME}" COMMAND "git" "rev-parse" "--abbrev-ref" "HEAD" OUTPUT_VARIABLE git_branch OUTPUT_STRIP_TRAILING_WHITESPACE)

if ("${git_branch}" STREQUAL "master")
    set(MICROBIT_DAL_VERSION_STRING "${YOTTA_MICROBIT_DAL_VERSION_STRING}")
else()
    set(MICROBIT_DAL_VERSION_STRING "${YOTTA_MICROBIT_DAL_VERSION_STRING}-${git_branch}-g${git_hash}")
endif()

set(MICROBIT_DAL_VERSION_FLAGS "-DMICROBIT_DAL_VERSION=\\\"${MICROBIT_DAL_VERSION_STRING}\\\"")

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${MICROBIT_DAL_VERSION_FLAGS}")

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

if(CMAKE_COMPILER_IS_GNUCC)
    message("suppressing ALL warnings from mbed-classic, ble, ble-nrf51822 & nrf51-sdk")
    target_compile_options(mbed-classic PRIVATE "-w")
    target_compile_options(ble PRIVATE "-w")
    target_compile_options(ble-nrf51822 PRIVATE "-w")
    target_compile_options(nrf51-sdk PRIVATE "-w")
endif()
