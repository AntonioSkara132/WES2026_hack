<<<<<<< HEAD
idf_build_get_property(target IDF_TARGET)
=======
set(COMPONENT_SRCS "app_main.c  gui.c Audio_play.c")
set(COMPONENT_ADD_INCLUDEDIRS "")
>>>>>>> 421efaa1d8044674cf6c60346a2590f4f05c27f4

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