cmake_minimum_required (VERSION 2.6)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(tools /opt/Xilinx/SDK/2017.2/gnu/aarch32/lin/gcc-arm-linux-gnueabi)
set(CMAKE_C_COMPILER ${tools}/bin/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${tools}/bin/arm-linux-gnueabihf-g++)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

project (Pulse_Acquire)

include_directories(${PROJECT_SOURCE_DIR}/include)

file(GLOB rp_pa_sources ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB rp_pa_headers ${PROJECT_SOURCE_DIR}/include/*.h)

add_executable(pacq pacq.c ${rp_pa_sources} ${rp_pa_headers})
target_link_libraries(pacq -lm -lpthread )
