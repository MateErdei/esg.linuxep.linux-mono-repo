add_library(ProxyUtilsLib SHARED
        ProxyUtils.h
        ProxyUtils.cpp
        )

target_link_libraries(ProxyUtilsLib PUBLIC filesystemimpl utilityimpl applicationconfigurationimpl logging obfuscationimpl)

target_include_directories(ProxyUtilsLib PUBLIC BEFORE ${NLOHMANN_JSON_INCLUDE_DIR})

SET_TARGET_PROPERTIES(ProxyUtilsLib
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS ProxyUtilsLib
        LIBRARY DESTINATION files/base/lib64)

install(TARGETS ProxyUtilsLib LIBRARY DESTINATION ${THININSTALLER_DEPS_DIR_LIB64})
