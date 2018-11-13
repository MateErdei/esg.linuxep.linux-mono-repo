cmake_minimum_required(VERSION 3.10)
project(sspl_template_plugin)

if(NOT DEFINED PLUGIN_NAME)
    set(PLUGIN_NAME TemplatePlugin)
endif()
string( TOLOWER ${PLUGIN_NAME} plugin_name)

if(NOT DEFINED PRODUCT_LINE_ID)
    set(PRODUCT_LINE_ID ServerProtectionLinux-Plugin-Template)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT DEFINED PRODUCT_NAME)
    set(PRODUCT_NAME ${PLUGIN_NAME})
endif()

if(NOT DEFINED DEFAULT_HOME_FOLDER)
    set(DEFAULT_HOME_FOLDER ${PRODUCT_NAME})
endif()

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

if(NOT DEFINED PLUGINAPIPATH)
    SET( PLUGINAPIPATH ${INPUT}/pluginapi)
endif()

set(pluginapiinclude ${PLUGINAPIPATH}/include)
set(pluginapilib ${PLUGINAPIPATH}/lib64/libpluginapi.so)

file(GLOB zmqlib ${PLUGINAPIPATH}/lib64/libzmq.so.*)
file(GLOB log4cpluslib ${PLUGINAPIPATH}/lib64/liblog4cplus-*.so.*)

add_subdirectory(installer)
add_subdirectory(distribution)
add_subdirectory(plugin)
