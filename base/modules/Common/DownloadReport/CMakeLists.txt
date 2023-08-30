
INCLUDE_DIRECTORIES(SYSTEM BEFORE PUBLIC ${PROTOBUF_INCLUDE_DIR})

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(ENV{LD_LIBRARY_PATH} ${Protobuf_LD_LIBRARY_PATH})
#set(Protobuf_PROTOC_EXECUTABLE ${CMAKE_BINARY_DIR}/protoc.sh)

if (EXISTS ${PROTOBUF_PROTOC_EXECUTABLE})
        message(STATUS "Found PROTOBUF Compiler: ${PROTOBUF_PROTOC_EXECUTABLE}")
else ()
        message(FATAL_ERROR "Could not find PROTOBUF Compiler")
endif ()

PROTOBUF_GENERATE_CPP(DOWNLOADER_REPORT_PROTO_SRC DOWNLOADER_REPORT_PROTO_H DownloadReport.proto)

add_library(downloadreportobject OBJECT
        DownloadReport.cpp
        DownloadReport.h
        DownloadReportParseException.h
        Logger.cpp
        Logger.h
        ${DOWNLOADER_REPORT_PROTO_SRC}
        ${DOWNLOADER_REPORT_PROTO_H}
        RepositoryStatus.cpp
        RepositoryStatus.h
        ProductInfo.h
        )

target_include_directories(downloadreportobject PUBLIC
        ${PROJECT_SOURCE_DIR}/modules
        ${CMAKE_BINARY_DIR}/modules
        ${LOG4CPLUS_INCLUDE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
        )

add_library(downloadreport SHARED)

target_link_libraries(downloadreport PRIVATE
        downloadreportobject
        )

install(TARGETS downloadreport
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
