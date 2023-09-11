add_library(flagutilsobject OBJECT
        FlagUtils.cpp
        FlagUtils.h
        Logger.cpp
        Logger.h)

target_include_directories(flagutilsobject PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})

include_directories(flagutilsobject BEFORE ${NLOHMANN_JSON_INCLUDE_DIR})

SET_TARGET_PROPERTIES(flagutilsobject PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS flagutilsobject LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)