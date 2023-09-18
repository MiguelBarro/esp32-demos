# List generated .proto files
file(GLOB PROTO_SOURCES "${CMAKE_SOURCE_DIR}/proto/*")

foreach(FILE IN LISTS PROTO_SOURCES)
    get_filename_component(PROTO_FILE_NAME "${FILE}" NAME_WLE)
    list(APPEND PROTO_GENERATED "${CMAKE_BINARY_DIR}/proto/${PROTO_FILE_NAME}.pb-c.c")
endforeach()

idf_component_register(
    SRCS
        "protobuf_example_main.cpp"
        ${PROTO_GENERATED}
    INCLUDE_DIRS
        "${CMAKE_BINARY_DIR}/proto"
    REQUIRES protobuf-c
)
