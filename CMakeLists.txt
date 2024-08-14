set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWOLFSSL_USER_SETTINGS")

idf_component_register(
    SRC_DIRS src
    EXCLUDE_SRCS
        "src/homekit_mdns.c"
        "src/homekit_mdns_debug.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES wolfssl esp_partition json http-parser mdns spi_flash
)

#if(CONFIG_HOMEKIT_SMALL)
#    list(APPEND EXTRA_WOLFSSL_COMPILE_OPTIONS -DCURVE25519_SMALL -DED25519_SMALL)
# endif()

idf_build_set_property(COMPILE_OPTIONS "${EXTRA_WOLFSSL_COMPILE_OPTIONS}" APPEND)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=unused-variable
    -DHOMEKIT_MAX_CLIENTS=${CONFIG_HOMEKIT_MAX_CLIENTS}
    -DSPIFLASH_BASE_ADDR=${CONFIG_HOMEKIT_SPI_FLASH_BASE_ADDR}
    -DESP_IDF
)

if(CONFIG_HOMEKIT_DEBUG)
    target_compile_options(${COMPONENT_LIB} PRIVATE -DHOMEKIT_DEBUG)
endif()
