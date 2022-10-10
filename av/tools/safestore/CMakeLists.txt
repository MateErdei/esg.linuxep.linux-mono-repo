add_executable(safestore_cmd
        SafestoreCmdline.cpp
        )

target_link_libraries(safestore_cmd PRIVATE
        safestoreimpl
        ${pluginapilib}
        )
target_include_directories(safestore_cmd PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${pluginapiinclude})

add_dependencies(safestore_cmd
        copy_libs)

SET_TARGET_PROPERTIES(safestore_cmd
        PROPERTIES
        INSTALL_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        )
