cmake_minimum_required(VERSION 3.24)

project(desktop-client
    VERSION 1.0.0
    DESCRIPTION "Desktop client demo matching the esp32 client demo"
    LANGUAGES C CXX)

# load dependencies
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
find_package(absl CONFIG REQUIRED)
find_package(Mosquitto MODULE REQUIRED)
find_package(protobuf CONFIG REQUIRED)

# generate protobuf proxies and stubs
file(GLOB PROTO_SOURCES ${CMAKE_CURRENT_LIST_DIR}/proto/*.proto)
protobuf_generate(
    PROTOS ${PROTO_SOURCES}
    PROTOC_OUT_DIR ${PROJECT_BINARY_DIR}
    OUT_VAR PROTO_PROXYSTUB
)

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif(MSVC)

# build the project
add_executable(${PROJECT_NAME} src/main.cpp ${PROTO_PROXYSTUB})
target_link_libraries(${PROJECT_NAME} PRIVATE absl::log absl::flags_parse Mosquitto::LibCpp protobuf::libprotobuf)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_BINARY_DIR}/proto)
target_compile_definitions(${PROJECT_NAME} PRIVATE ABSL_MIN_LOG_LEVEL=0)

install(TARGETS ${PROJECT_NAME})
