# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(IDF_TARGET "esp32" CACHE STRING "ESP chip selected as target")

set_property(CACHE IDF_TARGET PROPERTY STRINGS esp32 esp32s2 esp32s3 esp32c3 esp32c2 esp32c6 esp32h2)
get_property(ESP_VALID_TARGETS CACHE IDF_TARGET PROPERTY STRINGS)

message(STATUS ${ESP_VALID_TARGETS})

# Check the IDF_TARGET is valid
if(NOT IDF_TARGET IN_LIST ESP_VALID_TARGETS)
    message(WARNING "IDF_TARGET target value ${IDF_TARGET} is not a valid one.")
endif()

# Generate the message proxy-stub
find_package(protobuf REQUIRED)
find_package(protobuf-c REQUIRED)

# Include the toolchain file
file(TO_CMAKE_PATH "$ENV{IDF_PATH}/tools/cmake/toolchain-${IDF_TARGET}.cmake" ESP_TARGET_TOOLCHAIN)

#if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
#    set(CMAKE_TOOLCHAIN_FILE "${ESP_TARGET_TOOLCHAIN}")
#elseif(NOT CMAKE_TOOLCHAIN_FILE PATH_EQUAL ESP_TARGET_TOOLCHAIN)
#    message(WARNING "Provided toolchain file value ${CMAKE_TOOLCHAIN_FILE} is not the expected one for this target.")
#endif()

# Substitute by the above in CMake 3.24
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${ESP_TARGET_TOOLCHAIN}")
else()
    file(TO_CMAKE_PATH "${CMAKE_TOOLCHAIN_FILE}" REAL_CMAKE_TOOLCHAIN_FILE)
    if(NOT REAL_CMAKE_TOOLCHAIN_FILE STREQUAL ESP_TARGET_TOOLCHAIN)
        message(WARNING "Provided toolchain file value ${CMAKE_TOOLCHAIN_FILE} is not the expected one for this target.")
    endif()
endif()

include(${CMAKE_TOOLCHAIN_FILE})

# Define the project
project(protobuf_example C CXX)

# Include for ESP-IDF build system functions
include($ENV{IDF_PATH}/tools/cmake/idf.cmake)

# Create idf::{target} and idf::freertos static libraries
idf_build_process(${IDF_TARGET}
    # try and trim the build; additional components
    # will be included as needed based on dependency tree
    #
    # although esptool_py does not generate static library,
    # processing the component is needed for flashing related
    # targets and file generation
    COMPONENTS freertos esptool_py spi_flash protobuf-c
    SDKCONFIG ${CMAKE_CURRENT_LIST_DIR}/sdkconfig
    BUILD_DIR ${CMAKE_BINARY_DIR})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# GENERATE_TEST_SOURCES() function
# calls protoc with the protobuf-c plugin
# receives as argument:
#  + the .proto filename that must be in the proto dir
#  + source generated file
#  + header generated file
function(GENERATE_TEST_SOURCES SRC HDR PROTO_FILE)

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/proto")

    string(REGEX MATCHALL  "[A-Z]:[^;:]+;" PATH_VARIABLE "$ENV{PATH}" )
    foreach (itvar ${PATH_VARIABLE})
            string(REPLACE ";" "" itvar ${itvar})
            set(OS_PATH_VARIABLE "${OS_PATH_VARIABLE}\\;${itvar}" )
    endforeach( itvar )

    add_custom_target(${PROTO_FILE}
        COMMAND ${CMAKE_COMMAND}
                -E env PATH="$<TARGET_FILE_DIR:protobuf-c::protoc-gen-c>\\;${OS_PATH_VARIABLE}"
                ${PROTOBUF_PROTOC_EXECUTABLE}
                --plugin=$<TARGET_FILE_NAME:protobuf-c::protoc-gen-c> -I${CMAKE_SOURCE_DIR}/proto ${PROTO_FILE}
                --c_out=${CMAKE_CURRENT_BINARY_DIR}/proto
        COMMENT Running protoc on ${PROTO_FILE}
        BYPRODUCTS ${SRC} ${HDR}
    )

endfunction()

GENERATE_TEST_SOURCES(
    proto/gps.pb-c.c
    proto/gps.pb-c.h
    gps.proto
)

add_executable(${CMAKE_PROJECT_NAME}.elf
    "src/protobuf_example_main.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/proto/gps.pb-c.c"
)
add_dependencies(${CMAKE_PROJECT_NAME}.elf gps.proto)
target_include_directories(${CMAKE_PROJECT_NAME}.elf PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/proto)
target_link_libraries(${CMAKE_PROJECT_NAME}.elf idf::freertos idf::spi_flash idf::protobuf-c)

idf_build_executable(${CMAKE_PROJECT_NAME}.elf)
