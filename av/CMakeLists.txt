cmake_minimum_required(VERSION 3.10)
project(everest_exampleplugin)

set(PLUGIN_NAME ExamplePlugin)
string( TOLOWER ${PLUGIN_NAME} plugin_name)

set(CMAKE_CXX_STANDARD 11)

set(REL_DIST sdds)
set(DIST "${CMAKE_BINARY_DIR}/${REL_DIST}")
set(CMAKE_INSTALL_PREFIX "${REL_DIST}")

if(NOT DEFINED REDIST)
    set( REDIST /redist )
endif()
if(NOT DEFINED BINARIES_DIR)
    set( BINARIES_DIR ${REDIST}/binaries )
endif()
if(NOT DEFINED INPUT)
    set( INPUT ${BINARIES_DIR}/linux11/input )
endif()

SET( PLUGINAPIPATH ${INPUT}/pluginapi)
set(pluginapiinclude ${PLUGINAPIPATH}/include)
set(pluginapilib ${PLUGINAPIPATH}/lib64/libpluginapi.so)
set(zmqlib ${PLUGINAPIPATH}/lib64/libzmq.so.5.1.5)
set(log4cpluslib ${PLUGINAPIPATH}/lib64/liblog4cplus-1.2.so.5.1.4)

set( LOG4CPLUS_INPUT ${INPUT}/log4cplus )

set( LOG4CPLUS_INCLUDE_DIR ${LOG4CPLUS_INPUT}/include )

add_subdirectory(installer)
add_subdirectory(distribution)
add_subdirectory(LoggingAPI)
add_subdirectory(plugin)