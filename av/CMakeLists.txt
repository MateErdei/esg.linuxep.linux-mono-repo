cmake_minimum_required(VERSION 3.10)
project(sspl-plugin-mav)


set (STD_FILESYSTEM_IF_REQUIRED
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        $<$<CXX_COMPILER_ID:Clang>:stdc++fs>)

# set(SANITISE_FLAGS   "-static-libasan -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=shift-exponent -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=alignment  -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fsanitize=returns-nonnull-attribute -fsanitize=nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -fsanitize=signed-integer-overflow -fsanitize-address-use-after-scope ")
set(SANITISE_FLAGS "")
set(WARNING_FLAGS "-Wall -Wextra -Werror -Wnon-virtual-dtor -fstack-protector")

if (DEFINED ENV{USE_LIBFUZZER})
    message("fuzz mode")
    set(FUZZING_FLAGS -DUSING_LIBFUZZER=1 -g -O1 -fno-omit-frame-pointer -fsanitize=fuzzer,address,undefined,integer)
    add_compile_options(${FUZZING_FLAGS})
    add_link_options(${FUZZING_FLAGS})
endif (DEFINED ENV{USE_LIBFUZZER})

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
    set(PLUGIN_NAME av)
    endif()
endif()
string( TOLOWER ${PLUGIN_NAME} plugin_name)
string( TOUPPER ${PLUGIN_NAME} PLUGIN_NAME_UC)

if(NOT DEFINED PRODUCT_LINE_ID)
    set(PRODUCT_LINE_ID ServerProtectionLinux-Plugin-${PLUGIN_NAME_UC})
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

#Specify the feature list of the form "feature1, feature2, feature3"
#Set "" for no features when including in a component suite
set(FEATURE_LIST "AV")


set(CMAKE_CXX_STANDARD 17)

# Check process children and give detail for each leak
## --trace-children=yes
set(MEMORYCHECK_COMMAND_OPTIONS
        "${MEMORYCHECK_COMMAND_OPTIONS} --leak-check=full --error-exitcode=80")

set(MEMORYCHECK_COMMAND_OPTIONS
        "${MEMORYCHECK_COMMAND_OPTIONS} --gen-suppressions=all --suppressions=${CMAKE_SOURCE_DIR}/build/valgrind/suppressions.supp")


include (CTest)
enable_testing()

set(REL_DIST installset)
set(DIST "${CMAKE_BINARY_DIR}/${REL_DIST}")
set(CMAKE_INSTALL_PREFIX "${REL_DIST}")

if(NOT DEFINED INPUT)
    set( INPUT ${CMAKE_SOURCE_DIR}/redist )
endif()

if(NOT EXISTS ${INPUT})
    string( APPEND ERRORMESSAGE
            "To build using CLion you first need to:\n"
            "1. Install CI tools\n"
            "    sudo bash .../setup/SetupCIBuildScripts.sh\n"
            "1. Run the full build script with setup flag:\n"
            "    ./build.sh --setup\n"
            "After that you will be able to build using CLion")
    message( FATAL_ERROR ${ERRORMESSAGE})
endif()

if(NOT DEFINED PLUGINAPIPATH)
    SET( PLUGINAPIPATH ${INPUT}/pluginapi)
endif()

if(NOT DEFINED BOOSTPATH)
    SET( BOOSTPATH ${INPUT})
endif()

set(pluginapiinclude ${PLUGINAPIPATH}/include)
set(LOG4CPLUS_INCLUDE_DIR ${pluginapiinclude})
set(pluginapilib ${PLUGINAPIPATH}/lib64/libpluginapi.so)
set(testhelpersinclude ${PLUGINAPIPATH}/tests/include)
set(testhelperslib ${PLUGINAPIPATH}/tests/lib64/libsharedtesthelpers.so)

file(GLOB zmqlib ${PLUGINAPIPATH}/lib64/libzmq.so.*.*.*)
file(GLOB log4cpluslib ${PLUGINAPIPATH}/lib64/liblog4cplus-*.so.*.*.*)
file(GLOB protobuflib ${PLUGINAPIPATH}/lib64/libprotobuf.so.*.*.*)

if(NOT DEFINED SUSIPATH)
    SET( SUSIPATH ${INPUT}/susi_build)
endif()
SET(susi_include ${SUSIPATH}/include)
SET(susi_libdir ${SUSIPATH}/lib)

if(NOT EXISTS ${susi_include})
    string( APPEND ERRORMESSAGE
            "Failed to find SUSI includes: ${SUSIPATH}")
    message( FATAL_ERROR ${ERRORMESSAGE})
endif()
SET(SUSI_SDDS_PATH ${INPUT}/susi_sdds)

INCLUDE(CAPN)

set(CAPNPROTO_INPUT ${INPUT}/capnproto )
set(CAPNPROTO_INCLUDE_DIR ${CAPNPROTO_INPUT}/include)
set(CAPNPROTO_LIBRARY_DIR ${CAPNPROTO_INPUT}/lib)
set(CAPNPROTO_EXECUTABLE_DIR ${CAPNPROTO_INPUT}/bin)
set(CAPNPROTO_EXECUTABLE ${CAPNPROTO_INPUT}/bin/capnp)
set(CAPNPROTO_LD_LIBRARY_PATH ${CAPNPROTO_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH} )
set( ENV{LD_LIBRARY_PATH} ${CAPNPROTO_LD_LIBRARY_PATH} )

message(NOTICE CAPNPROTO_LD_LIBRARY_PATH=${CAPNPROTO_LD_LIBRARY_PATH})

set(OPENSSL_INPUT ${INPUT}/openssl)
set(OPENSSL_INCLUDE_DIR  ${OPENSSL_INPUT}/include64)
find_library(CRYPTO_LIBRARY
        NAMES crypto
        PATHS ${OPENSSL_INPUT}/lib64
        NO_DEFAULT_PATH
        )

GET_FILENAME_COMPONENT(OPENSSLCRYPTO_LIBRARY_ABSOLUTE ${CRYPTO_LIBRARY} REALPATH)

add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)
add_subdirectory(tools/avscanner/mountinfoimpl)
