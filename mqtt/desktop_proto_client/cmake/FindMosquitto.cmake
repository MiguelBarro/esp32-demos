# - Try to find Mosquitto
# Once done, this will define
#
#  Mosquitto::Lib - The Mosquitto library target
#  Mosquitto::LibCpp - The Mosquitto CPP library target

find_path(MOSQUITTO_INCLUDE_DIR NAMES mosquitto.h PATH_SUFFIXES devel)
find_library(MOSQUITTO_LIBRARY NAMES mosquitto PATH_SUFFIXES devel)
find_library(MOSQUITTO_LIBRARY_CPP NAMES mosquittopp PATH_SUFFIXES devel)

find_program(MOSQUITTO_SERVER NAMES mosquitto mosquitto.exe)
find_program(MOSQUITTO_PUB NAMES mosquitto_pub mosquitto_pub.exe)
find_program(MOSQUITTO_SUB NAMES mosquitto_sub mosquitto_sub.exe)

get_filename_component(MOSQUITTO_BINARY_DIR "${MOSQUITTO_SERVER}" PATH)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Mosquitto
    DEFAULT_MSG
    MOSQUITTO_BINARY_DIR
    MOSQUITTO_PUB
    MOSQUITTO_SUB
    MOSQUITTO_LIBRARY
    MOSQUITTO_LIBRARY_CPP
    MOSQUITTO_INCLUDE_DIR)

if(MOSQUITTO_FOUND)
    add_library(Mosquitto::Lib UNKNOWN IMPORTED)
    set_target_properties(Mosquitto::Lib PROPERTIES
        IMPORTED_LOCATION "${MOSQUITTO_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${MOSQUITTO_INCLUDE_DIR}"
    )

    add_library(Mosquitto::LibCpp UNKNOWN IMPORTED)
    set_target_properties(Mosquitto::LibCpp PROPERTIES
        IMPORTED_LOCATION "${MOSQUITTO_LIBRARY_CPP}"
        INTERFACE_INCLUDE_DIRECTORIES "${MOSQUITTO_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(
    MOSQUITTO_BINARY_DIR
    MOSQUITTO_PUB
    MOSQUITTO_SUB
    MOSQUITTO_LIBRARY
    MOSQUITTO_LIBRARY_CPP
    MOSQUITTO_INCLUDE_DIR
)
