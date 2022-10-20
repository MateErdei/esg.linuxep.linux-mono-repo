add_executable(safestore_print_tool
        SafeStoreDatabasePrintTool.cpp
        )

target_link_libraries(safestore_print_tool PRIVATE
        safestoreimpl
        ${pluginapilib}
        )
target_include_directories(safestore_print_tool PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${pluginapiinclude})

add_dependencies(safestore_print_tool
        copy_libs)

SET_TARGET_PROPERTIES(safestore_print_tool
        PROPERTIES
        INSTALL_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs:/opt/sophos-spl/plugins/av/lib64/"
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS safestore_print_tool
        DESTINATION ${CMAKE_BINARY_DIR}/tap_test_output)
