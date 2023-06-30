add_library(ProxyUtilsLibobject OBJECT
        ProxyUtils.h
        ProxyUtils.cpp
        Logger.cpp
        Logger.h
        )

target_include_directories(ProxyUtilsLibobject PUBLIC BEFORE
        ${CMAKE_SOURCE_DIR}
        ${LOG4CPLUS_INCLUDE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/modules/Common
        )

add_library(ProxyUtilsLib SHARED $<TARGET_OBJECTS:ProxyUtilsLibobject>)

target_link_libraries(ProxyUtilsLib PUBLIC filesystemimpl utilityimpl applicationconfigurationimpl logging obfuscationimpl)

target_include_directories(ProxyUtilsLib PUBLIC BEFORE ${NLOHMANN_JSON_INCLUDE_DIR})

SET_TARGET_PROPERTIES(ProxyUtilsLib
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS ProxyUtilsLib
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
