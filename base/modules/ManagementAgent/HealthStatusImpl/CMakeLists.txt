add_library(HealthStatusImpl SHARED
        HealthStatus.h
        HealthStatus.cpp
        HealthTask.h
        HealthTask.cpp
        SerialiseFunctions.cpp)
target_include_directories(HealthStatusImpl PUBLIC  ../../Common ${LOG4CPLUS_INCLUDE_DIR} ../../../thirdparty/nlohmann-json)
target_link_libraries(HealthStatusImpl PUBLIC applicationconfigurationimpl managementagentloggerimpl taskqueueimpl)

SET_TARGET_PROPERTIES( HealthStatusImpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS HealthStatusImpl
        LIBRARY DESTINATION files/base/lib64)
