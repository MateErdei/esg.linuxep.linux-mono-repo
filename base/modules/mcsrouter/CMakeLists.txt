cmake_minimum_required(VERSION 3.5)

file(GLOB_RECURSE mcs_router_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ./*.py )

file(GLOB_RECURSE mcs_router_DEPS
        ./*.py )

add_custom_target(echo_mcs
        COMMAND echo mcs_router_SRC=${mcs_router_SRC}
        COMMAND echo mcs_router_DEPS=${mcs_router_DEPS}
        )

add_custom_target(mcsrouter ALL
        DEPENDS mcsrouter.zip
        )

add_custom_command(OUTPUT mcsrouter.zip
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND zip ${CMAKE_BINARY_DIR}/mcsrouter.zip ${mcs_router_SRC}
        DEPENDS ${mcs_router_DEPS}
        )

install(FILES ${CMAKE_BINARY_DIR}/mcsrouter.zip
        DESTINATION files/base/lib64)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/engine/mcs_rootca.crt
        DESTINATION files/base/mcs/certs)
