SophosAddTest(processlivequerypolicytests
        processlivequerypolicytests.cpp
        PROJECTS pluginimpl
        LIBS ${pluginapilib} ${testhelperslib} ${LOG4CPLUS_LIBRARY}
        INC_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/modules ${pluginapiinclude} ${testhelpersinclude} ${LOG4CPLUS_INCLUDE_DIR}
        )

SET_TARGET_PROPERTIES( processlivequerypolicytests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )