idf_component_register(
    SRCS
        "protobuf_example_main.c"
        "${CMAKE_BINARY_DIR}/proto/gps.pb-c.c"
    INCLUDE_DIRS
        "${CMAKE_BINARY_DIR}/proto"
    REQUIRES protobuf-c
)
