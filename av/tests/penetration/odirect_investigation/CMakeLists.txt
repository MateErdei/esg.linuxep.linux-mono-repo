add_executable(odirect_client
        odirect_client.cpp)
target_link_libraries(odirect_client  PUBLIC unixsocket pthread)
target_include_directories(odirect_client PUBLIC ${CMAKE_SOURCE_DIR}/modules)
SET_TARGET_PROPERTIES( odirect_client
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BUILD_RPATH}"
        INSTALL_RPATH "$ORIGIN:${CMAKE_INSTALL_RPATH}"
        )

add_executable(odirect_server
        odirect_server.cpp)
target_link_libraries(odirect_server  PUBLIC unixsocket)
target_include_directories(odirect_server PUBLIC ${CMAKE_SOURCE_DIR}/modules)
SET_TARGET_PROPERTIES( odirect_server
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BUILD_RPATH}"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib64:${CMAKE_INSTALL_RPATH}"
        )
