cmake_minimum_required(VERSION 3.10)
project(liveresponse)


set(SANITISE_FLAGS   "-static-libasan -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=shift-exponent -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=alignment  -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fsanitize=returns-nonnull-attribute -fsanitize=nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -fsanitize=signed-integer-overflow -fsanitize-address-use-after-scope ")
set(WARNING_FLAGS "-Wall -Wextra -Werror -fstack-protector")

set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${SANITISE_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${SANITISE_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/build-files/cmake")
set(RELEASE_PACKAGE_PATH "${CMAKE_SOURCE_DIR}/../../linux-release-package.xml")
include(SophosAddTest)
include(SophosAddLibrary)

if(NOT DEFINED PLUGIN_NAME)
    # developers may just add a file called PLUGINNAME to get all the variables
    # set as the ./build.sh set them out.
    if ( EXISTS "${CMAKE_SOURCE_DIR}/PLUGINNAME" )
        file(READ "${CMAKE_SOURCE_DIR}/PLUGINNAME" PLUGIN_NAME_FROMFILE)
        string( STRIP ${PLUGIN_NAME_FROMFILE} PLUGIN_NAME)
    else()
    set(PLUGIN_NAME liveresponse)
    endif()
endif()

if( NOT DEFINED LIVETERMINAL)
    set(LIVETERMINAL /build/redist/x86_64-unknown-linux-gnu/release/sophos-live-terminal)
    message(WARNING "sophos-live-terminal path not given in the configuration. Defaulting to: ${LIVETERMINAL}")
endif()

string( TOLOWER ${PLUGIN_NAME} plugin_name)

if(NOT DEFINED PRODUCT_LINE_ID)
    set(PRODUCT_LINE_ID ServerProtectionLinux-Plugin-${PLUGIN_NAME})

endif()

execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT DEFINED PRODUCT_NAME)
    set(PRODUCT_NAME ${PLUGIN_NAME})
endif()

if(NOT DEFINED DEFAULT_HOME_FOLDER)
    set(DEFAULT_HOME_FOLDER ${PRODUCT_NAME})
endif()

#Specify the feature list of the form "feature1, feature2, feature3"
#Set "" for no features when including in a component suite
set(FEATURE_LIST "LIVETERMINAL")


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

if(NOT DEFINED INPUT)
    set( INPUT /build/input )
endif()
if(NOT DEFINED REDIST)
    set( REDIST /build/redist )
endif()

if(NOT EXISTS ${INPUT})
    string( APPEND ERRORMESSAGE
            "To build using CLion you first need to:\n"
            "1. Fetch inputs in source folder by running the following in the root directory of this repo:\n"
            "    python3 -m build_scripts.artisan_fetch linux-release-package.xml\n"
            "2. Run the full build:\n"
            "    python3 -m build_scripts.artisan_build linux-release-package.xml\n"
            "After that you will be able to build using CLion")
    message( FATAL_ERROR ${ERRORMESSAGE})
endif()

if(NOT DEFINED PLUGINAPIPATH)
    SET( PLUGINAPIPATH ${REDIST}/pluginapi)
endif()

if(NOT GCC_INPUT)
    set (GCC_INPUT "/build/input/gcc")
endif(NOT GCC_INPUT)

set(pluginapiinclude ${PLUGINAPIPATH}/include)
set(pluginapilib ${PLUGINAPIPATH}/lib64/libpluginapi.so)
set(DISTRIBUTION_TOOL_DIR ${PLUGINAPIPATH}/distribution)
set(testhelpersinclude ${PLUGINAPIPATH}/tests/include)
set(testhelperslib ${PLUGINAPIPATH}/tests/lib64/libsharedtesthelpers.so)

set(GTEST_INPUT ${REDIST}/googletest)
set(JSON_SINGLE_INPUT ${REDIST}/nlohmann-json/single_include)

file(GLOB expatlib ${PLUGINAPIPATH}/lib64/libexpat.so*)
file(GLOB zmqlib ${PLUGINAPIPATH}/lib64/libzmq.so*)
set(log4cpluslib ${PLUGINAPIPATH}/lib64/liblog4cplus.so)
file(GLOB protobuflib ${PLUGINAPIPATH}/lib64/libprotobuf.so.*)

set(GTEST_INCLUDE ${GTEST_INPUT}/include)
set(GTEST_MAIN_LIBRARY ${GTEST_INPUT}/googletest_main.a)
if (NOT EXISTS ${GTEST_MAIN_LIBRARY})
    message("Library not found: ${GTEST_MAIN_LIBRARY}")
endif ()

include(cppcheck.cmake)
add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)
