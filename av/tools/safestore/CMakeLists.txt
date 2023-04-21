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
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "/opt/sophos-spl/plugins/av/lib64/"
        )

install(TARGETS safestore_print_tool
        DESTINATION ${CMAKE_BINARY_DIR}/tap_test_output)
