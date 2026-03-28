idf_build_get_property(target IDF_TARGET)

if(CONFIG_EXAMPLE_IPV4)
    set(tcp_client_ip tcp_client_v4.c)
else()
    set(tcp_client_ip tcp_client_v6.c)
endif()

set(COMPONENT_SRCS
    app_main.c
    gui.c
    ${tcp_client_ip}
)

idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS "")