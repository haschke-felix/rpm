cmake_minimum_required(VERSION 2.8.12)
project(rpm)

set(ARDUINO_EXTRA_LIBRARIES_PATH 
  $ENV{HOME}/Arduino/libraries
  ${ARDUINO_SDK_PATH}/hardware/arduino/avr/libraries
)

set(ARDUINO_DEFAULT_BOARD uno) # Default Board ID, when not specified
set(ARDUINO_DEFAULT_PORT /dev/ttyUSB0) # Default Port, when not specified

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
generate_arduino_firmware(${PROJECT_NAME} SRCS rpm.ino)
