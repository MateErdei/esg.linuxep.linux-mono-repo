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
#        ${CMAKE_CURRENT_BINARY_DIR})

SET_TARGET_PROPERTIES(safestore_cmd
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN"
        )
