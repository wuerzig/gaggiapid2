
idf_component_register(SRCS "main.cpp" "network.cpp" "http_handlers.cpp" "dns_server.c"
                    INCLUDE_DIRS "include" "."
                    REQUIRES esp-idf-cxx max31865 driver esp_netif nvs_flash esp_wifi esp_http_server
                    EMBED_FILES root.html)


# target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE=1)
#target_compile_options(${COMPONENT_LIB} PUBLIC -std=c++20)
