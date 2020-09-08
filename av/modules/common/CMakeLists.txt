add_library(common SHARED
        AbortScanException.h
        Logger.cpp
        Logger.h
        StringUtils.cpp
        StringUtils.h)

SET (BOOST_LIBRARYDIR  ${BOOSTPATH}/boost/lib64)
SET (BOOST_INCLUDEDIR ${BOOSTPATH}/boost/include)
SET (BOOST_ROOT ${BOOSTPATH}/boost)

find_package( Boost REQUIRED COMPONENTS locale system)

target_link_libraries(common  PUBLIC
        scanmessages
        datatypes
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        ${pluginapilib}
        ${CRYPTO_LIBRARY}
        ${Boost_LIBRARIES}
        )

target_include_directories(common PUBLIC ${pluginapiinclude} ${CMAKE_SOURCE_DIR}/modules ${OPENSSL_INCLUDE_DIR})
target_include_directories(common SYSTEM PRIVATE ${BOOSTPATH})

install(TARGETS common
        LIBRARY DESTINATION files/plugins/${PLUGIN_NAME}/lib64)

SET_TARGET_PROPERTIES(common
        PROPERTIES
        BUILD_RPATH "$ORIGIN:${CMAKE_BINARY_DIR}/libs"
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../../../base/lib64"
        )