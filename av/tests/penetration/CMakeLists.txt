add_executable(attack_client
        attack_client.cpp)
target_link_libraries(attack_client  PUBLIC unixsocket)
target_include_directories(attack_client PUBLIC ${CMAKE_SOURCE_DIR}/modules)

SET_TARGET_PROPERTIES( attack_client
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BUILD_RPATH}:/opt/sophos-spl/plugins/av/lib64"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_INSTALL_RPATH}:/opt/sophos-spl/plugins/av/lib64"
        )

add_executable(async_attack_client
        async_attack_client.cpp)
target_link_libraries(async_attack_client  PUBLIC unixsocket)
target_include_directories(async_attack_client PUBLIC ${CMAKE_SOURCE_DIR}/modules)

SET_TARGET_PROPERTIES( async_attack_client
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BUILD_RPATH}:/opt/sophos-spl/plugins/av/lib64"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib64:$ORIGIN/../../../base/lib64:${CMAKE_INSTALL_RPATH}:/opt/sophos-spl/plugins/av/lib64"
        )

add_subdirectory(odirect_investigation)
