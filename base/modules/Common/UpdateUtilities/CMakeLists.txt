add_library(updateutilitiesobject OBJECT
        ConfigurationDataUtil.cpp
        ConfigurationDataUtil.h
        DownloadReports.cpp
        DownloadReports.h
        InstalledFeatures.cpp
        InstalledFeatures.h
        Logger.cpp
        Logger.h
        )

target_include_directories(updateutilitiesobject PUBLIC
        ${PROJECT_SOURCE_DIR}/modules
        ${CMAKE_BINARY_DIR}/modules
        ${LOG4CPLUS_INCLUDE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
        )

add_library(updateutilities SHARED)

target_link_libraries(updateutilities PRIVATE
        updateutilitiesobject
        )

install(TARGETS updateutilities
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
