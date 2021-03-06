cmake_minimum_required(VERSION 2.8)

#set (CPU_FREQ "16000000UL")
#set (CPU "atmega328p")
#set (CPU_AVRDUDE "m328p")
set (CPU_FREQ "18432000UL")
set (CPU "atmega88p")
set (CPU_AVRDUDE "m88")

set(TARGET firmware.elf)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

include_directories("../include/avr/" "../include/avr/c++/" "../common/include" "../common/include/uclibcpp")
link_directories("../common/")

file(GLOB Project_SOURCES *.cpp)
#file(GLOB Project_SOURCES test/hello.c)
file(GLOB Project_HEADER s*.hpp)

set (CMAKE_C_COMPILER "/usr/bin/avr-gcc") 
set (CMAKE_C_FLAGS "-ffunction-sections -std=c99 -fdata-sections -O2 -Wl,--relax,--gc-sections -DF_CPU=${CPU_FREQ} -mmcu=${CPU}")

set (CMAKE_CXX_COMPILER "/usr/bin/avr-g++") 
set (CMAKE_CXX_FLAGS "-ffunction-sections -fpermissive -fdata-sections -std=c++11 -O2 -Wl,--relax,--gc-sections -DF_CPU=${CPU_FREQ} -mmcu=${CPU}")

link_libraries(${TARGET} "drivers")

#set (CMAKE_C_COMPILER "/usr/bin/avr-gcc") 
#set (CMAKE_C_FLAGS "-ffunction-sections -fdata-sections -O2 -Wl,--relax,--gc-sections -DF_CPU=16000000UL -mmcu=atmega328p")
add_custom_target(install
    COMMAND avr-objcopy -j .text -j .data -O ihex ${TARGET} ${TARGET}.hex
    COMMAND avr-size -C -x ${TARGET}
		COMMAND sudo avrdude -p ${CPU_AVRDUDE} -c usbasp -e -U flash:w:${TARGET}.hex
    DEPENDS ${TARGET}
)

# Executable
add_executable(${TARGET} ${Project_SOURCES} ${Project_HEADERS})

target_link_libraries(${TARGET})
