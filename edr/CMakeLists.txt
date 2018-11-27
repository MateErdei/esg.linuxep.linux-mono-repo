cmake_minimum_required(VERSION 3.10)
project(sspl_template_plugin)


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/buildconfig")
include(SophosAddTest)

if(NOT DEFINED PLUGIN_NAME)
    # developers may just add a file called PLUGINNAME to get all the variables
    # set as the ./build.sh set them out.
    if ( EXISTS "${CMAKE_SOURCE_DIR}/PLUGINNAME" )
        file(READ "${CMAKE_SOURCE_DIR}/PLUGINNAME" PLUGIN_NAME_FROMFILE)
        string( STRIP ${PLUGIN_NAME_FROMFILE} PLUGIN_NAME)
    else()
    set(PLUGIN_NAME TemplatePlugin)
    endif()
endif()
string( TOLOWER ${PLUGIN_NAME} plugin_name)

if(NOT DEFINED PRODUCT_LINE_ID)
    set(PRODUCT_LINE_ID ServerProtectionLinux-${PLUGIN_NAME})

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

set(CMAKE_CXX_STANDARD 17)
enable_testing()
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
SET( PLUGINAPIPATH /tmp/pluginapi)
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
add_subdirectory(tests)
