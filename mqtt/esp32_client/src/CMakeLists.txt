idf_component_register(SRCS "mqtt_example_main.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES esp_netif esp_eth ethernet_init nvs_flash mqtt mdns)
