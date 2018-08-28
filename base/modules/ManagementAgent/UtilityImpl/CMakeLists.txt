cmake_minimum_required(VERSION 3.5)
add_library(managementagentutilityimpl SHARED
        PolicyFileUtilities.cpp
        PolicyFileUtilities.h
        )
target_include_directories(managementagentutilityimpl PUBLIC  ../../Common)
target_link_libraries(managementagentutilityimpl PUBLIC utilityimpl)

SET_TARGET_PROPERTIES( managementagentutilityimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS managementagentutilityimpl
        LIBRARY DESTINATION files/base/lib64)