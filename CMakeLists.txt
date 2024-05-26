set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DWOLFSSL_USER_SETTINGS")
set(WOLFSSL_COMPONENT_NAME "wolfssl")

# Optional Staging instance use of wolfssl, the component is called "mywolfssl"
if( "$ENV{IDF_COMPONENT_REGISTRY_URL}" STREQUAL "https://components-staging.espressif.com" )
    # When set to a specific value, IDF_COMPONENT_REGISTRY_URL indicates the component is used for testing / preview
    if(  ( NOT ("${managed_components}"                  STREQUAL ""  )) OR
         ( NOT ("${component_manager_interface_version}" STREQUAL ""  )) OR
         (     ("${IDF_COMPONENT_MANAGER}"               STREQUAL "1" ))    )
        # Alternate naming for staging component
        message(STATUS "*********************************************************************************")
        message(STATUS "*********************************************************************************")
        message(STATUS "*                                                                               *")
        message(STATUS "*        NOTICE: Using wolfSSL preview test library called mywolfssl from       *")
        message(STATUS "*                                                                               *")
        message(STATUS "*     IDF_COMPONENT_REGISTRY_URL  =  https://components-staging.espressif.com   *")
        message(STATUS "*                                                                               *")
        message(STATUS "*********************************************************************************")
        message(STATUS "*********************************************************************************")
        set(WOLFSSL_COMPONENT_NAME "mywolfssl")
    else()
        # Detected IDF_COMPONENT_REGISTRY_URL as staging, but could not confirm component manager
        message(STATUS "*********************************************************************************")
        message(STATUS "*********************************************************************************")
        message(STATUS "*                                                                               *")
        message(STATUS "* WARNING: could not detect component manager, using wolfSSL component wolfssl  *")
        message(STATUS "*                                                                               *")
        message(STATUS "*     IDF_COMPONENT_REGISTRY_URL =   https://components-staging.espressif.com   *")
        message(STATUS "*                                                                               *")
        message(STATUS "*********************************************************************************")
        message(STATUS "*********************************************************************************")
        set(WOLFSSL_COMPONENT_NAME "wolfssl")
    endif()
else()
    set(WOLFSSL_COMPONENT_NAME "wolfssl")
endif()

message(STATUS "WOLFSSL_COMPONENT_NAME= ${WOLFSSL_COMPONENT_NAME}")
idf_component_register(
    SRC_DIRS src
    EXCLUDE_SRCS
        "src/homekit_mdns.c"
        "src/homekit_mdns_debug.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "src"
    REQUIRES esp_partition json http-parser mdns spi_flash "${WOLFSSL_COMPONENT_NAME}"
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
