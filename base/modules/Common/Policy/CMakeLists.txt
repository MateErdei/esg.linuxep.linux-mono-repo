add_library(policy SHARED
        ALCPolicy.cpp
        ALCPolicy.h
        )

include_directories(policy BEFORE ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})
target_link_libraries(policy PUBLIC obfuscationimpl xmlutilities)

SET_TARGET_PROPERTIES(policy
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS policy
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

add_dependencies(policy copy_libs)
