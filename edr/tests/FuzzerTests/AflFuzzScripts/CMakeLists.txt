
sophos_add_exe(processlivequerypolicytests
        processlivequerypolicytests.cpp
        EXTRA_PROJECTS pluginimpl
        EXTRA_LIBS ${pluginapilib}
        EXTRA_INCLUDES ${CMAKE_CURRENT_BINARY_DIR} ${pluginapiinclude}
        )

target_link_libraries(processlivequerypolicytests PRIVATE
        ${LOG4CPLUS_LIBRARY}
        )

SET_TARGET_PROPERTIES( processlivequerypolicytests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

target_include_directories(processlivequerypolicytests PUBLIC ${CMAKE_SOURCE_DIR}/modules  ${LOG4CPLUS_INCLUDE_DIR}   ${CMAKE_CURRENT_BINARY_DIR})
