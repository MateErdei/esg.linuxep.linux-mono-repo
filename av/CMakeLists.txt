cmake_minimum_required(VERSION 3.10)
project(sspl-plugin-mav)

if (POLICY CMP0115) # Visibility
    cmake_policy(SET CMP0115 NEW)
endif (POLICY CMP0115)

# Poison un-safe standard library functions
set(UNSAFE_FUNCS "strerror" "strcpy" "wcscpy" "stpcpy" "wcpcpy" "scanf" "sscanf" "vscanf" "fwscanf" "swscanf" "wscanf"
        "gets(" "puts(" "strcat" "wcscat" "wcrtomb" "wctob" "sprintf" "vsprintf" "vfprintf" "asprintf" "vasprintf"
        "wcsncpy" "strtok" "wcstok" "mbrlen" "mbsrtowcs" "mbrtowc" "wcrtomb" "wcsrtombs" "asctime" "localtime"
        "longjmp" "siglongjmp" "setjmp" "sigsetjmp" "mallopt" "mktemp" "tmpnam" "tempnam" "getwd" "getlogin" "getpass"
        "cuserid" "rexec" "rexec_af" "rand" "srand" "setlocale" "localeconv")
foreach(FUNC ${UNSAFE_FUNCS})
    message(STATUS "Checking source code for un-safe standard library function: ${FUNC}")

    execute_process (
            COMMAND grep -rwI ${FUNC} modules/ products/ tests/
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE POISON_FUNCS
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE EXIT_CODE
    )

    if (NOT EXIT_CODE EQUAL 1)
        message(FATAL_ERROR "Unsafe standard library functions in use: ${POISON_FUNCS}")
    endif()
endforeach()

set (STD_FILESYSTEM_IF_REQUIRED
        $<$<AND:$<CXX_COMPILER_ID:GNU>,$<VERSION_LESS:$<CXX_COMPILER_VERSION>,9.0>>:stdc++fs>
        $<$<CXX_COMPILER_ID:Clang>:stdc++fs>)

# set(SANITISE_FLAGS   "-static-libasan -fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=shift-exponent -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=alignment  -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fsanitize=returns-nonnull-attribute -fsanitize=nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -fsanitize=signed-integer-overflow -fsanitize-address-use-after-scope ")
set(SANITISE_FLAGS "")
set(WARNING_FLAGS "-Wall -Wextra -Werror -Wnon-virtual-dtor -fstack-protector -Wtrampolines")

if (DEFINED ENV{USE_LIBFUZZER})
    message("fuzz mode")
    set(WARNING_FLAGS "-Wall -Wextra -Werror -Wnon-virtual-dtor -fstack-protector")
    set(FUZZING_FLAGS -DUSING_LIBFUZZER=1 -g -O1 -Wall -Wextra -Werror -fstack-protector -fno-omit-frame-pointer -Wno-unused-result -Wno-unused-parameter -Wno-unused-variable -Wno-unused-private-field -fsanitize=fuzzer,address,undefined)
    add_compile_options(${FUZZING_FLAGS})
    add_link_options(${FUZZING_FLAGS})
    set(LIBCXX_LIB_PATH "-L /build/input/gcc/lib64")
endif (DEFINED ENV{USE_LIBFUZZER})

set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${SANITISE_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${SANITISE_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${LIBCXX_LIB_PATH}")


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

if(NOT DEFINED AV_PRETTY_NAME)
    set(AV_PRETTY_NAME "Sophos Linux AntiVirus")
endif()

#Specify the feature list of the form "feature1, feature2, feature3"
#Set "" for no features when including in a component suite
set(FEATURE_LIST "AV")


set(CMAKE_CXX_STANDARD 17)

set(MEMORYCHECK_SUPPRESSIONS_FILE "${CMAKE_SOURCE_DIR}/build/valgrind/suppressions.supp")

# Check process children and give detail for each leak
string(APPEND MEMORYCHECK_COMMAND_OPTIONS " --trace-children=yes")
string(APPEND MEMORYCHECK_COMMAND_OPTIONS " --trace-children-skip=*/scheduled_file_walker_launcher") # for TestScanScheduler

string(APPEND MEMORYCHECK_COMMAND_OPTIONS " --leak-check=full")
string(APPEND MEMORYCHECK_COMMAND_OPTIONS " --max-stackframe=2621440") # for sync_versioned_files

string(APPEND MEMORYCHECK_COMMAND_OPTIONS " --error-exitcode=80")
string(APPEND MEMORYCHECK_COMMAND_OPTIONS " --gen-suppressions=all")

# string(APPEND MEMORYCHECK_COMMAND_OPTIONS " --verbose")
string(APPEND MEMORYCHECK_COMMAND_OPTIONS " --show-error-list=yes") # show used suppressions
string(APPEND MEMORYCHECK_COMMAND_OPTIONS " --fullpath-after=${CMAKE_SOURCE_DIR}/")

include (CTest)
enable_testing()

set(REL_DIST installset)
set(DIST "${CMAKE_BINARY_DIR}/${REL_DIST}")
set(CMAKE_INSTALL_PREFIX "${REL_DIST}")
set(OUTPUT "${CMAKE_SOURCE_DIR}/output")

if(NOT DEFINED INPUT)
    set( INPUT ${CMAKE_SOURCE_DIR}/input )
endif()

if(NOT DEFINED REDIST)
    set( REDIST ${CMAKE_SOURCE_DIR}/redist )
endif()

if(NOT EXISTS ${REDIST})
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
    SET( PLUGINAPIPATH ${REDIST}/pluginapi)
endif()

if(NOT DEFINED BOOSTPATH)
    SET( BOOSTPATH ${REDIST})
endif()
SET(Boost_NO_WARN_NEW_VERSIONS ON)

set(pluginapiinclude ${PLUGINAPIPATH}/include)
set(LOG4CPLUS_INCLUDE_DIR ${pluginapiinclude})
set(pluginapilib ${PLUGINAPIPATH}/lib64/libpluginapi.so)
set(testhelpersinclude ${PLUGINAPIPATH}/tests/include)
set(testhelperslib ${PLUGINAPIPATH}/tests/lib64/libsharedtesthelpers.so)
set(GTEST_INPUT ${INPUT}/googletest)

file(GLOB zmqlib ${PLUGINAPIPATH}/lib64/libzmq.so.*.*.*)
file(GLOB log4cpluslib ${PLUGINAPIPATH}/lib64/liblog4cplus-*.so.*.*.*)
file(GLOB protobuflib ${PLUGINAPIPATH}/lib64/libprotobuf.so.*.*.*)

set(GTEST_INCLUDE ${GTEST_INPUT}/gtest-headers)
find_library(GTEST_LIBRARY
        NAMES gtest
        PATHS ${GTEST_INPUT}/gtest-libs
        NO_DEFAULT_PATH)
find_library(GTEST_MAIN_LIBRARY
        NAMES gtest_main
        PATHS ${GTEST_INPUT}/gtest-libs
        NO_DEFAULT_PATH)
set( GMOCK_INCLUDE ${GTEST_INPUT}/gmock-headers )
find_library(GMOCK_LIBRARY
        NAMES gmock
        PATHS ${GTEST_INPUT}/gmock-libs
        NO_DEFAULT_PATH)

if(NOT DEFINED SUSIPATH)
    SET( SUSIPATH ${REDIST}/susi_build)
endif()
SET(susi_include ${SUSIPATH}/include)
SET(susi_libdir ${SUSIPATH}/lib)

if(NOT EXISTS ${susi_include})
    string( APPEND ERRORMESSAGE
            "Failed to find SUSI includes: ${SUSIPATH}")
    message( FATAL_ERROR ${ERRORMESSAGE})
endif()
SET(SUSI_SDDS_PATH ${REDIST}/susi_sdds)

INCLUDE(CAPN)

set(CAPNPROTO_INPUT ${REDIST}/capnproto )
set(CAPNPROTO_INCLUDE_DIR ${CAPNPROTO_INPUT}/include)
set(CAPNPROTO_LIBRARY_DIR ${CAPNPROTO_INPUT}/lib64)
set(CAPNPROTO_LIBRARIES "${CAPNPROTO_LIBRARY_DIR}/libcapnpc.a"
                        "${CAPNPROTO_LIBRARY_DIR}/libcapnp-rpc.a"
                        "${CAPNPROTO_LIBRARY_DIR}/libcapnp.a"
                        "${CAPNPROTO_LIBRARY_DIR}/libkj-async.a"
                        "${CAPNPROTO_LIBRARY_DIR}/libkj.a")
set(CAPNPROTO_EXECUTABLE_DIR ${CAPNPROTO_INPUT}/bin)
set(CAPNPROTO_EXECUTABLE ${CAPNPROTO_INPUT}/bin/capnp)
set(CAPNPROTO_LD_LIBRARY_PATH ${CAPNPROTO_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH}:$ENV{LIBRARY_PATH} )
set( ENV{LD_LIBRARY_PATH} ${CAPNPROTO_LD_LIBRARY_PATH} )

message("CAPNPROTO_LD_LIBRARY_PATH=${CAPNPROTO_LD_LIBRARY_PATH}")

set(OPENSSL_INPUT ${REDIST}/openssl)
set(OPENSSL_INCLUDE_DIR  ${OPENSSL_INPUT}/include64)
find_library(CRYPTO_LIBRARY
        NAMES crypto
        PATHS ${OPENSSL_INPUT}/lib64
        NO_DEFAULT_PATH
        )

set(SAFESTORE_INPUT ${INPUT}/safestore_input)
execute_process (
        COMMAND ln -sf  libsafestore.so.1 libsafestore.so
        WORKING_DIRECTORY ${SAFESTORE_INPUT})

find_library(SAFESTORE_LIB
        NAMES safestore
        PATHS ${SAFESTORE_INPUT}
        NO_DEFAULT_PATH
        )

set(ZLIB_INPUT ${REDIST}/zlib)
set(ZLIB_INCLUDE_DIR ${ZLIB_INPUT}/include)
find_library(ZLIB_LIBRARY
        NAMES z
        PATHS ${ZLIB_INPUT}/lib64
        NO_DEFAULT_PATH
        )

set(JSON_INPUT ${pluginapiinclude}/thirdparty/nlohmann-json)

GET_FILENAME_COMPONENT(OPENSSLCRYPTO_LIBRARY_ABSOLUTE ${CRYPTO_LIBRARY} REALPATH)

add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)
add_subdirectory(tools)
add_subdirectory(tools/avscanner/mountinfoimpl)

# Must come last
add_subdirectory(PostInstall)