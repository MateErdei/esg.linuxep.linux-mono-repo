add_library(mcsimpl SHARED
        IAdapter.h
        AgentAdapter.h
        AgentAdapter.cpp
        MCSHttpClient.cpp
        MCSHttpClient.h
        Logger.cpp
        Logger.h
        MCSApiCalls.cpp
        MCSApiCalls.h)

include_directories(mcsimpl BEFORE ${LOG4CPLUS_INCLUDE_DIR} ${CURL_INCLUDE_DIR})
target_link_libraries(mcsimpl PUBLIC HttpRequestsImpl obfuscationimpl osutilitiesimpl ${OPENSSLCRYPTO_LIBRARY})

SET_TARGET_PROPERTIES(mcsimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS mcsimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(mcsimpl copy_libs)
