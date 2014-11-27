cmake_minimum_required(VERSION 2.8)

include_directories("${CMAKE_CURRENT_LIST_DIR}/arch/" "${CMAKE_CURRENT_LIST_DIR}/arch/m328p/" ${CMAKE_CURRENT_LIST_DIR})

file(GLOB_RECURSE SOURCE_FILES ${CMAKE_CURRENT_LIST_DIR}/*.c)
#./*.c ./kernel/arch/m328p/*.c boards/*.c crypto/*.c crypto/aes/*.c disp/*.c hid/*.c io/*.c motors/*.c net/*.c radio/*.c rfid/*.c sensors/*.c tty/*.c) 
file(GLOB_RECURSE HEADER_FILES ${CMAKE_CURRENT_LIST_DIR}/*.h)

set(Kernel_SOURCES ${SOURCE_FILES})
set(Kernel_HEADERS ${HEADER_FILES})
