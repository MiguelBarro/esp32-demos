# Platformio cannot handle CMake code using toolchains. Only the plain ESP-IDF framework projects.
if (NOT DEFINED ENV{PLATFORMIO_IDE})
    include(CMakeToolchain.cmake)
    return()
endif()

cmake_minimum_required(VERSION 3.16)

# generate the message proxy-stub
# find_package must use CONFIGs because any instrospection will
# trigger the language before the toolchain and spoil platformio build
find_package(protobuf CONFIG REQUIRED)
find_package(protobuf-c CONFIG REQUIRED)

get_target_property(PROTOBUF_PROTOC_EXECUTABLE protobuf::protoc LOCATION)

if(WIN32)
    get_target_property(PLUGIN_PATH_VARIABLE protobuf-c::protoc-gen-c LOCATION)
    get_filename_component(PLUGIN_PATH_VARIABLE "${PLUGIN_PATH_VARIABLE}" PATH)
    set(ENV{PATH} "$ENV{PATH};${PLUGIN_PATH_VARIABLE}")
endif(WIN32)

# GENERATE_TEST_SOURCES() function
# receives as argument the .proto filename that must be in the proto dir
function(GENERATE_TEST_SOURCES PROTO_FILE)
    if(WIN32)
        set(PLUGIN_NAME protoc-gen-c.exe)
    else(WIN32)
        set(PLUGIN_NAME protoc-gen-c)
    endif(WIN32)

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/proto")

    execute_process(
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --plugin=${PLUGIN_NAME} -I${CMAKE_SOURCE_DIR}/proto ${PROTO_FILE}
                --c_out=${CMAKE_CURRENT_BINARY_DIR}/proto
    )
endfunction()

file(GLOB PROTO_SOURCES "${CMAKE_CURRENT_LIST_DIR}/proto/*")
foreach(FILE IN LISTS PROTO_SOURCES)
    GENERATE_TEST_SOURCES(${FILE})
endforeach()

set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/src")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(protobuf_example)
