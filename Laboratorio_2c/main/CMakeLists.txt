set(requires "")
idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    list(APPEND requires esp_stubs esp-tls esp_http_server protocol_examples_common nvs_flash led)
endif()
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES ${requires}
                    EMBED_FILES 
            "${CMAKE_CURRENT_SOURCE_DIR}/index.html"
            "${CMAKE_CURRENT_SOURCE_DIR}/style.css")
