# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# generate the message proxy-stub
find_package(protobuf REQUIRED)
find_package(protobuf-c REQUIRED)

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

    execute_process(
        COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} --plugin=${PLUGIN_NAME} -I${CMAKE_SOURCE_DIR}/proto ${PROTO_FILE}
                --c_out=${CMAKE_CURRENT_BINARY_DIR}/proto
    )
endfunction()

GENERATE_TEST_SOURCES(gps.proto)

set(EXTRA_COMPONENT_DIRS "${CMAKE_CURRENT_LIST_DIR}/src")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(protobuf_example)
