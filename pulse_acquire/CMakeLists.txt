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

set(BUILD_SCRIPT ${PROJECT_BINARY_DIR}/bgen.sh)
#set(rp_pa_build_h ${PROJECT_BINARY_DIR}/build.h)

file(GLOB rp_pa_sources ${PROJECT_SOURCE_DIR}/src/*.c)
file(GLOB rp_pa_headers ${PROJECT_SOURCE_DIR}/include/*.h)
#file(GLOB rp_pa_build_h ${PROJECT_BINARY_DIR}/build.h)

add_custom_target(generate_build
    COMMAND ${BUILD_SCRIPT}
    DEPENDS ${BUILD_SCRIPT}
    VERBATIM
    )

add_executable(pacq pacq.c ${rp_pa_sources} ${rp_pa_headers} ${PROJECT_BINARY_DIR}/build.h)
add_dependencies(pacq generate_build)
target_link_libraries(pacq -lm -lpthread )

#set(PACQ_SCRIPTS
#    bgen.sh
#    build.h
#  )

#foreach(_script ${PACQ_SCRIPTS})
#  configure_file(
#    ${PROJECT_SOURCE_DIR}/${_script}
#    ${PROJECT_BINARY_DIR}/${_script}
#    COPYONLY
#    )
#endforeach()

configure_file( ${PROJECT_SOURCE_DIR}/bgen.sh
                ${PROJECT_BINARY_DIR}/bgen.sh
    COPYONLY
)

configure_file( ${PROJECT_SOURCE_DIR}/build_h
                ${PROJECT_BINARY_DIR}/build.h
    COPYONLY
)
