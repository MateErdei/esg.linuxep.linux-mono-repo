cmake_minimum_required(VERSION 3.10)
project(sspl_event_journaler)


option(BUILD_FUZZ_TESTS "Turn on the build of executables for fuzz test" OFF)
if (BUILD_FUZZ_TESTS)
    set(SANITISE_FLAGS   "")
    set(SANITIZE_EXE_FLAGS "")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(FUZZER_COMPILER_FLAGS "-fsanitize=address,fuzzer-no-link" )
        set(FUZZER_LINKER_FLAGS "-fsanitize=address,fuzzer" )
        set(NO_FUZZING_FLAGS "-fno-sanitize=fuzzer")
        set(BUILD_FOR_DEVELOPER_DEBUG  False)
    else()
        set(FUZZER_COMPILER_FLAGS "-fsanitize=address" )
        set(FUZZER_LINKER_FLAGS "-fsanitize=address" )
        set(NO_FUZZING_FLAGS "")
        set(BUILD_FOR_DEVELOPER_DEBUG  True)
    endif()
    set(WARNING_FLAGS "")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${SANITIZE_EXE_FLAGS} ${FUZZER_LINKER_FLAGS}")
else()
    set(SANITISE_FLAGS   "-static-libasan -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=shift-exponent -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=alignment  -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fsanitize=returns-nonnull-attribute -fsanitize=nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -fsanitize=signed-integer-overflow -fsanitize-address-use-after-scope ")
    set(NO_FUZZING_FLAGS "")
    set(FUZZER_COMPILER_FLAGS "" )
    set(FUZZER_LINKER_FLAGS "" )
    set(WARNING_FLAGS "-Wall -Wextra -Werror -fstack-protector")
endif()

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
    set(PLUGIN_NAME eventjournaler)
    endif()
endif()
if(NOT DEFINED PRODUCT_PRETTY_NAME)
    set(PRODUCT_PRETTY_NAME "Sophos Linux Event Journaler")
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
set(FEATURE_LIST "CORE")


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
    set( INPUT /build/redist )
endif()

if(NOT EXISTS ${INPUT})
    string( APPEND ERRORMESSAGE
            "To build using CLion you first need to:\n"
            "1. Fetch inputs in source folder by running:\n"
            "    ./build.sh --setup\n"
            "After that you will be able to build using CLion")
    message( FATAL_ERROR ${ERRORMESSAGE})
endif()

if(NOT DEFINED PLUGINAPIPATH)
    SET( PLUGINAPIPATH ${INPUT}/pluginapi)
endif()
set(DISTRIBUTION_TOOL_DIR ${PLUGINAPIPATH}/distribution)

set(pluginapiinclude ${PLUGINAPIPATH}/include)
set(pluginapilib ${PLUGINAPIPATH}/lib64/libpluginapi.so)
set(testhelpersinclude ${PLUGINAPIPATH}/tests/include)
set(testhelperslib ${PLUGINAPIPATH}/tests/lib64/libsharedtesthelpers.so)
set(GTEST_INPUT ${INPUT}/googletest)

set(JOURNAL_INPUT ${INPUT}/JournalLib)
set(XZUTILS_INPUT ${INPUT}/lzma)

set(CAPNPROTO_INPUT ${INPUT}/capnproto )
set(CAPNPROTO_INCLUDE_DIR ${CAPNPROTO_INPUT}/include)
set(CAPNPROTO_LIBRARY_DIR ${CAPNPROTO_INPUT}/lib64)
set(CAPNPROTO_EXECUTABLE_DIR ${CAPNPROTO_INPUT}/bin)
set(CAPNPROTO_EXECUTABLE ${CAPNPROTO_INPUT}/bin/capnp)
set(CAPNPROTO_LD_LIBRARY_PATH ${CAPNPROTO_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH} )
set( ENV{LD_LIBRARY_PATH} ${CAPNPROTO_LD_LIBRARY_PATH} )
set( XZUTILS_INCLUDE_DIR ${XZUTILS_INPUT}/src/liblzma/api)
message( ${CAPNPROTO_LD_LIBRARY_PATH})

set(JOURNAL_INCLUDE_DIR ${JOURNAL_INPUT}/include)
find_library(JOURNAL_LIBRARY
        NAMES libJournalLib.a
        PATHS ${INPUT}/JournalLib/lib
        NO_DEFAULT_PATH)

find_library(LZMA_LIBRARY
        NAMES liblzma.so
        PATHS ${XZUTILS_INPUT}
        NO_DEFAULT_PATH)
execute_process(COMMAND chrpath -r \$ORIGIN ${LZMA_LIBRARY} COMMAND_ERROR_IS_FATAL ANY)

set(GTEST_INCLUDE ${GTEST_INPUT}/include)
set(GTEST_MAIN_LIBRARY ${GTEST_INPUT}/googletest_main.a)
if (NOT EXISTS ${GTEST_MAIN_LIBRARY})
    message("Library not found: ${GTEST_MAIN_LIBRARY}")
endif ()

file(GLOB expatlib ${PLUGINAPIPATH}/lib64/libexpat.so*)
file(GLOB zmqlib ${PLUGINAPIPATH}/lib64/libzmq.so*)
set(log4cpluslib ${PLUGINAPIPATH}/lib64/liblog4cplus.so)
file(GLOB protobuflib ${PLUGINAPIPATH}/lib64/libprotobuf.so.*)
file(GLOB opensslcryptolib ${PLUGINAPIPATH}/lib64/libcrypto.so.*)

include(cppcheck.cmake)
add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)
