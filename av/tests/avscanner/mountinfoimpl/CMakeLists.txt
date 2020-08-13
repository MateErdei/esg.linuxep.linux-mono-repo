
add_executable(PrintMounts
        PrintMounts.cpp
        )
target_link_libraries(PrintMounts PRIVATE
        mountinfoimpl
        )
target_include_directories(PrintMounts PUBLIC
        ${CMAKE_SOURCE_DIR}/modules
        ${pluginapiinclude}
        ${CMAKE_CURRENT_BINARY_DIR})
