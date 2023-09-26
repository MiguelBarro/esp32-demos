cmake_minimum_required(VERSION 3.24)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Mosquitto MODULE REQUIRED)

project(desktop-client
    VERSION 1.0.0
    DESCRIPTION "Desktop client demo matching the esp32 client demo"
    LANGUAGES C CXX)

add_executable(${PROJECT_NAME} src/main.cpp)
target_link_libraries(${PROJECT_NAME} Mosquitto::LibCpp)

install(TARGETS ${PROJECT_NAME})


message(STATUS "${MOSQUITTO_BINARY_DIR}")
message(STATUS "${MOSQUITTO_PUB}")
message(STATUS "${MOSQUITTO_SUB}")
message(STATUS "${MOSQUITTO_LIBRARY}")
message(STATUS "${MOSQUITTO_LIBRARY_CPP}")
message(STATUS "${MOSQUITTO_INCLUDE_DIR}")
