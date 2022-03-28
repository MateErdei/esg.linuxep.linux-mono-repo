add_library(mcsimpl SHARED
        MCSHttpClient.cpp
        MCSHttpClient.h
        Logger.cpp
        Logger.h
        )

include_directories(mcsimpl BEFORE ${LOG4CPLUS_INCLUDE_DIR} ${CURL_INCLUDE_DIR})
target_link_libraries(mcsimpl PUBLIC HttpRequestsImpl)

SET_TARGET_PROPERTIES(mcsimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS mcsimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(mcsimpl copy_libs)
