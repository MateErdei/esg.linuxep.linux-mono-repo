SophosAddTest(processlivequerypolicytests
        processlivequerypolicytests.cpp
        PROJECTS pluginimpl
        INC_DIRS ${testhelpersinclude}
        )

SET_TARGET_PROPERTIES( processlivequerypolicytests
        PROPERTIES
        INSTALL_RPATH "${CMAKE_BINARY_DIR}/libs"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )