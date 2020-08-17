
add_executable(PrintMounts
        PrintMounts.cpp
        )
target_link_libraries(PrintMounts PRIVATE
        mountinfoimpl
        ${pluginapilib}
        )
target_include_directories(PrintMounts PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${pluginapiinclude}
        ${CMAKE_CURRENT_BINARY_DIR})

SET_TARGET_PROPERTIES(PrintMounts
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN"
        )
