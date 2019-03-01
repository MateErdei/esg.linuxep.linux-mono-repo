cmake_minimum_required(VERSION 3.10)
project(sspl_template_plugin)


set(SANITISE_FLAGS   "-static-libasan -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=shift-exponent -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=alignment  -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fsanitize=returns-nonnull-attribute -fsanitize=nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -fsanitize=signed-integer-overflow -fsanitize-address-use-after-scope ")
set(WARNING_FLAGS "-Wall -Wextra -Werror -fstack-protector")

set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${SANITISE_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${SANITISE_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/buildconfig")
include(SophosAddTest)
include(SophosAddLibrary)

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
    set(PRODUCT_LINE_ID ServerProtectionLinux-Plugin-${PLUGIN_NAME})

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

# Check process children and give detail for each leak
## --trace-children=yes
set(MEMORYCHECK_COMMAND_OPTIONS
        "${MEMORYCHECK_COMMAND_OPTIONS} --leak-check=full --error-exitcode=80")

set(MEMORYCHECK_COMMAND_OPTIONS
        "${MEMORYCHECK_COMMAND_OPTIONS} --gen-suppressions=all --suppressions=${CMAKE_SOURCE_DIR}/build/valgrind/suppressions.supp")


include (CTest)
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

if(NOT DEFINED PLUGINAPIPATH)
    SET( PLUGINAPIPATH ${INPUT}/pluginapi)
endif()

set(pluginapiinclude ${PLUGINAPIPATH}/include)
set(pluginapilib ${PLUGINAPIPATH}/lib64/libpluginapi.so)
set(testhelpersinclude ${PLUGINAPIPATH}/tests/include)
set(testhelperslib ${PLUGINAPIPATH}/tests/lib64/libsharedtesthelpers.so)

file(GLOB zmqlib ${PLUGINAPIPATH}/lib64/libzmq.so.*)
file(GLOB log4cpluslib ${PLUGINAPIPATH}/lib64/liblog4cplus-*.so.*)
file(GLOB protobuflib ${PLUGINAPIPATH}/lib64/libprotobuf.so.*)

add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)
