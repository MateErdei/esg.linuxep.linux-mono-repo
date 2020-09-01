add_library(securityutilsobject OBJECT
        ProcessSecurityUtils.h
        ProcessSecurityUtils.cpp
        )

target_include_directories(securityutilsobject PUBLIC ${CMAKE_SOURCE_DIR})

add_library(securityutils SHARED $<TARGET_OBJECTS:securityutilsobject>)
target_link_libraries(securityutils PUBLIC filesystemimpl)

SET_TARGET_PROPERTIES(securityutils
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS securityutils
        LIBRARY DESTINATION files/base/lib64)