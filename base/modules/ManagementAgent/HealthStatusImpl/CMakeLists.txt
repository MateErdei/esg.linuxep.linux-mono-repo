add_library(HealthStatusImpl SHARED
        HealthStatus.h
        HealthStatus.cpp
        ../HealthTask/HealthTask.h
        ../HealthTask/HealthTask.cpp
        SerialiseFunctions.h
        SerialiseFunctions.cpp)
target_include_directories(HealthStatusImpl PUBLIC  ../../Common ${LOG4CPLUS_INCLUDE_DIR} ${NLOHMANN_JSON_INCLUDE_DIR})
target_link_libraries(HealthStatusImpl PUBLIC applicationconfigurationimpl managementagentloggerimpl taskqueueimpl)

SET_TARGET_PROPERTIES( HealthStatusImpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS HealthStatusImpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
