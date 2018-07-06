# This file is no longer auto-generated to make the repository builds with GCC
# and ARMCC no matter what.

cmake_minimum_required(VERSION 2.8.12)

enable_language(ASM)

set(YOTTA_AUTO_MICROBIT-DAL_CPP_FILES
    "core/MemberFunctionCallback.cpp"
    "core/MicroBitCompat.cpp"
    "core/MicroBitDevice.cpp"
    "core/MicroBitFiber.cpp"
    "core/MicroBitFont.cpp"
    "core/MicroBitHeapAllocator.cpp"
    "core/MicroBitListener.cpp"
    "core/MicroBitSystemTimer.cpp"
    "core/MicroBitUtil.cpp"

    "types/CoordinateSystem.cpp"
    "types/ManagedString.cpp"
    "types/MicroBitEvent.cpp"
    "types/MicroBitImage.cpp"
    "types/PacketBuffer.cpp"
    "types/RefCounted.cpp"

    "drivers/DynamicPwm.cpp"
    "drivers/MicroBitAccelerometer.cpp"
    "drivers/MicroBitButton.cpp"
    "drivers/MicroBitCompass.cpp"
    "drivers/MicroBitCompassCalibrator.cpp"
    "drivers/MicroBitDisplay.cpp"
    "drivers/MicroBitI2C.cpp"
    "drivers/MicroBitIO.cpp"
    "drivers/MicroBitLightSensor.cpp"
    "drivers/MicroBitMessageBus.cpp"
    "drivers/MicroBitMultiButton.cpp"
    "drivers/MicroBitPin.cpp"
    "drivers/MicroBitQuadratureDecoder.cpp"
    "drivers/MicroBitRadio.cpp"
    "drivers/MicroBitRadioDatagram.cpp"
    "drivers/MicroBitRadioEvent.cpp"
    "drivers/MicroBitSerial.cpp"
    "drivers/MicroBitStorage.cpp"
    "drivers/MicroBitThermometer.cpp"
    "drivers/TimedInterruptIn.cpp"
    "drivers/MicroBitFlash.cpp"
    "drivers/MicroBitFile.cpp"
    "drivers/MicroBitFileSystem.cpp"
    "drivers/FXOS8700.cpp"
    "drivers/MMA8653.cpp"
    "drivers/MAG3110.cpp"
    "drivers/LSM303Accelerometer.cpp"
    "drivers/LSM303Magnetometer.cpp"
    "drivers/MicroBitMemoryMap.cpp"

    "bluetooth/MicroBitAccelerometerService.cpp"
    "bluetooth/MicroBitBLEManager.cpp"
    "bluetooth/MicroBitButtonService.cpp"
    "bluetooth/MicroBitDFUService.cpp"
    "bluetooth/MicroBitEddystone.cpp"
    "bluetooth/MicroBitEventService.cpp"
    "bluetooth/MicroBitIOPinService.cpp"
    "bluetooth/MicroBitLEDService.cpp"
    "bluetooth/MicroBitMagnetometerService.cpp"
    "bluetooth/MicroBitTemperatureService.cpp"
    "bluetooth/MicroBitUARTService.cpp"
    "bluetooth/MicroBitPartialFlashingService.cpp"

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
