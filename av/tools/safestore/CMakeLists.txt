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
        INSTALL_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        )
