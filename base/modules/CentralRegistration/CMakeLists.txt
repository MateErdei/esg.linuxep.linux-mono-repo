cmake_minimum_required(VERSION 3.8)

add_library(centralregistrationimpl SHARED
        CentralRegistration.cpp
        CentralRegistration.h
        Logger.cpp
        Logger.h
        Main.cpp
        Main.h
        MessageRelayExtractor.h
        MessageRelaySorter.h
        )

target_include_directories(centralregistrationimpl SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${BOOST_INCLUDE_DIR} ${CURL_INCLUDE_DIR} )
include_directories(centralregistrationimpl BEFORE ../../thirdparty/nlohmann-json)

target_link_libraries(centralregistrationimpl PUBLIC logging HttpRequestsImpl osutilitiesimpl  mcsimpl ${LOG4CPLUS_LIBRARY} curlwrapper)

SET_TARGET_PROPERTIES( centralregistrationimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS centralregistrationimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

add_dependencies(centralregistrationimpl copy_libs)
