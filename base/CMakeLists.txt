cmake_minimum_required(VERSION 3.8)
project(everest_base CXX)

option(test "Build all tests." ON) # Makes boolean 'test' available.
option(BUILD_FUZZ_TESTS "Turn on the build of executables for fuzz test" OFF)
option(PythonCoverage "Turn of the instrumentation for producing python coverage results" OFF)
option(STRACE_SUPPORT "Prevent sanitize options that break strace" OFF)
set(CMAKE_CXX_STANDARD 17)
set(SOPHOS_SPL_USER "sophos-spl-user")
set(SOPHOS_SPL_GROUP "sophos-spl-group")
set(SOPHOS_SPL_IPC_GROUP "sophos-spl-ipc")
set(SOPHOS_SPL_NETWORK "sophos-spl-network")   #comms network facing user
set(SOPHOS_SPL_LOCAL "sophos-spl-local")       #comms product facing user
set(SOPHOS_SPL_UPDATESCHEDULER "sophos-spl-updatescheduler")  #user which updatescheduler runs as

if (BUILD_FUZZ_TESTS)
    set(SANITISE_FLAGS "")
    set(SANITIZE_EXE_FLAGS "")
    set(FUZZER_COMPILER_FLAGS "-fsanitize=address,fuzzer-no-link")
    set(FUZZER_LINKER_FLAGS "-fsanitize=address,fuzzer")
    set(NO_FUZZING_FLAGS "-fno-sanitize=fuzzer")
    set(WARNING_FLAGS "")
elseif (STRACE_SUPPORT)
    set(SANITISE_FLAGS "-fsanitize=address -fsanitize=undefined -fsanitize=shift-exponent -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=alignment  -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fsanitize=returns-nonnull-attribute -fsanitize=nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -fsanitize=signed-integer-overflow -fsanitize-address-use-after-scope ")
    set(NO_FUZZING_FLAGS "")
    set(FUZZER_COMPILER_FLAGS "")
    set(FUZZER_LINKER_FLAGS "")
    set(WARNING_FLAGS "-Wall -Wextra -Werror -fstack-protector-all")
else()
    set(SANITISE_FLAGS   "-fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=shift-exponent -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=alignment  -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fsanitize=returns-nonnull-attribute -fsanitize=nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -fsanitize=signed-integer-overflow -fsanitize-address-use-after-scope ")
    set(NO_FUZZING_FLAGS "")
    set(FUZZER_COMPILER_FLAGS "" )
    set(FUZZER_LINKER_FLAGS "" )
    set(WARNING_FLAGS "-Wall -Wextra -Werror -fstack-protector-all")
endif()

set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${FUZZER_COMPILER_FLAGS} ${WARNING_FLAGS} ${SANITISE_FLAGS}")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-z,relro -Wl,-z,now")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${SANITIZE_EXE_FLAGS} ${FUZZER_LINKER_FLAGS} ${SANITISE_FLAGS}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(GCOV_ACTIVE OFF)
if (NOT NO_GCOV STREQUAL true AND CMAKE_BUILD_TYPE STREQUAL Debug)
    # Set GCOV_ACTIVE so that we can use it to determine if GCOV is active later
    set(GCOV_ACTIVE ON)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -lgcov")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
    message("GCOV ACTIVE")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/buildconfig")
set(CMAKE_INSTALL_RPATH "$ORIGIN")


# Check process children and give detail for each leak
## --trace-children=yes
set(MEMORYCHECK_COMMAND_OPTIONS
        "${MEMORYCHECK_COMMAND_OPTIONS} --leak-check=full --error-exitcode=80")

set(MEMORYCHECK_COMMAND_OPTIONS
        "${MEMORYCHECK_COMMAND_OPTIONS} --gen-suppressions=all --suppressions=${CMAKE_SOURCE_DIR}/build/valgrind/suppressions.supp")

set(PYTHON_EXECUTABLE python3)

include (CTest)
enable_testing()

if(NOT GCC_INPUT)
    set (GCC_INPUT "/build/input/gcc")
endif(NOT GCC_INPUT)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

if(NOT DEFINED PRODUCT_NAME)
    set(PRODUCT_NAME "Sophos Server Protection Linux - Base")
endif()
if(NOT DEFINED PRODUCT_LINE_ID)
    set(PRODUCT_LINE_ID "ServerProtectionLinux-Base")
endif()
if(NOT DEFINED DEFAULT_HOME_FOLDER)
    set(DEFAULT_HOME_FOLDER "sspl-base")
endif()

#Specify the feature list of the form "feature1, feature2, feature3"
#Set "" for no features when including in a component suite
set(FEATURE_LIST "CORE")

find_package(PkgConfig)
FIND_PACKAGE ( Threads REQUIRED )

if(NOT DEFINED INPUT)
    set( INPUT ${CMAKE_SOURCE_DIR}/redist )
endif()

if(NOT EXISTS ${INPUT})
    string( APPEND ERRORMESSAGE
    "To build using CLion you first need to:\n"
    "1. Fetch inputs in source folder by running:\n"
    "    python3 -m build_scripts.artisan_fetch build/release-package.xml\n"
    "2. Run the full build script with no build flag:\n"
    "    ./build.sh --no-build\n"
    "After that you will be able to build using CLion")
    message( FATAL_ERROR ${ERRORMESSAGE})
endif()

## Find 3rd party libraries
set( BOOST_INPUT ${INPUT}/boost )
set( CURL_INPUT ${INPUT}/curl )
set( OPENSSL_INPUT ${INPUT}/openssl )
set( EXPAT_INPUT ${INPUT}/expat )
set( ZLIB_INPUT ${INPUT}/zlib )
set( LOG4CPLUS_INPUT ${INPUT}/log4cplus )

set( BOOST_INCLUDE ${BOOST_INPUT}/include )

if(NOT DEFINED VERSIG_INPUT)
    set( VERSIG_INPUT ${INPUT}/versig)
endif()

## Allow this to be redirected for SUL development
if(NOT DEFINED SUL_INPUT)
    set( SUL_INPUT ${INPUT}/SUL )
endif()

set( SUL_LIBRARY_DIR ${SUL_INPUT}/lib64 )
set( SUL_LIBRARY_DIRS STRING ${CMAKE_BINARY_DIR}/libs:${SUL_LIBRARY_DIR}:${BOOST_INPUT}/lib64:${CURL_INPUT}/lib64:${OPENSSL_INPUT}/lib64 FORCE)

set( BOOST_INCLUDE_DIR ${BOOST_INPUT}/include )

set( PROTOBUF_INPUT ${INPUT}/protobuf )
set(Protobuf_root ${PROTOBUF_INPUT}/install64)
set(Protobuf_INCLUDE_DIR ${Protobuf_root}/include)
set(PROTOBUF_PROTOC_EXECUTABLE ${Protobuf_root}/bin/protoc)
set(Protobuf_PROTOC_LIBRARY ${Protobuf_root}/lib/libprotoc.so ${Protobuf_root}/lib/libprotobuf.so )
set(Protobuf_LITE_LIBRARY  ${Protobuf_root}/lib/libprotobuf-lite.so )
set(Protobuf_LIBRARY ${Protobuf_root}/lib/libprotobuf.so )
set(Protobuf_DEBUG 1)
set(Protobuf_LD_LIBRARY_PATH ${Protobuf_root}/lib:$ENV{LD_LIBRARY_PATH} )
set( ENV{LD_LIBRARY_PATH} ${Protobuf_LD_LIBRARY_PATH} )
INCLUDE(FindProtobuf)
INCLUDE(CAPN)
#FIND_PACKAGE(Protobuf REQUIRED )
configure_file(build/protoc.sh.in protoc.sh)

set(CAPNPROTO_INPUT ${INPUT}/capnproto )
set(CAPNPROTO_INCLUDE_DIR ${CAPNPROTO_INPUT}/include)
set(CAPNPROTO_LIBRARY_DIR ${CAPNPROTO_INPUT}/lib64)
set(CAPNPROTO_EXECUTABLE_DIR ${CAPNPROTO_INPUT}/bin)
set(CAPNPROTO_EXECUTABLE ${CAPNPROTO_INPUT}/bin/capnp)
set(CAPNPROTO_LD_LIBRARY_PATH ${CAPNPROTO_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH} )
set( ENV{LD_LIBRARY_PATH} ${CAPNPROTO_LD_LIBRARY_PATH} )

message( ${CAPNPROTO_LD_LIBRARY_PATH})

set( ZEROMQ_INPUT ${INPUT}/zeromq )
## use pkg-config to get hints for 0mq locations
#pkg_check_modules(PC_ZeroMQ QUIET zmq)

## load in pkg-config support
## use the hint from above to find where 'zmq.hpp' is located
find_path(ZeroMQ_INCLUDE_DIR
        NAMES zmq.h
        PATHS ${ZEROMQ_INPUT}/include
        )

## use the hint from above to find the location of libzmq
find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS ${ZEROMQ_INPUT}/lib
        )

find_path(EXPAT_INCLUDE_DIR
        NAMES expat.h
        PATHS ${EXPAT_INPUT}/include
        NO_DEFAULT_PATH
        )
find_library(EXPAT_LIBRARY
        NAMES expat
        PATHS ${EXPAT_INPUT}/lib64
        NO_DEFAULT_PATH
        )

set( LOG4CPLUS_INCLUDE_DIR ${LOG4CPLUS_INPUT}/include )

find_library(LOG4CPLUS_LIBRARY
        NAMES log4cplus
        PATHS ${LOG4CPLUS_INPUT}/lib
        )

# ensure valid log4cplus to work with fork
file(STRINGS ${LOG4CPLUS_LIBRARY} log4cplusStrings)
string(FIND "${log4cplusStrings}" "implicit initialization is turned off" DisabledImplicitInitialization)
if(${DisabledImplicitInitialization} EQUAL -1)
    string( APPEND ERRORMESSAGE
    "Log4cplus needs to be built with --disable-implicit-initialization to work in the presence of fork. \n"
    "Make sure you have fetched the inputs and build with build.sh at least once\n"
    "1. Fetch inputs in source folder by running:\n"
    "    python3 -m build_scripts.artisan_fetch build/release-package.xml\n"
    "2. Run the full build script with no build flag:\n"
    "    ./build.sh --no-build\n")
    message( FATAL_ERROR ${ERRORMESSAGE})
endif()


set( OPENSSL_INCLUDE_DIR  ${OPENSSL_INPUT}/include64)
find_library(OPENSSL_LIBRARY
        NAMES ssl
        PATHS ${OPENSSL_INPUT}/lib64
        NO_DEFAULT_PATH
        )
find_library(OPENSSLCRYPTO_LIBRARY
        NAMES crypto
        PATHS ${OPENSSL_INPUT}/lib64
        NO_DEFAULT_PATH
        )

set( CURL_INCLUDE_DIR ${CURL_INPUT}/include64 )
find_library(CURL_LIBRARY
        NAMES curl
        PATHS ${CURL_INPUT}/lib64
        NO_DEFAULT_PATH
        )

find_library(ZLIB_LIBRARY
        NAMES z
        PATHS ${ZLIB_INPUT}/lib64
        NO_DEFAULT_PATH
        )

## Allow this to be redirected for python development
if(NOT DEFINED PYTHON_INPUT)
    set( PYTHON_INPUT ${INPUT}/python)
endif()

## Allow this to be redirected for pathtools development
if(NOT DEFINED WATCHDOG_INPUT)
    set( WATCHDOG_INPUT ${INPUT}/watchdog)
endif()

## Allow this to be redirected for watchdog development
if(NOT DEFINED PATHTOOLS_INPUT)
    set( PATHTOOLS_INPUT ${INPUT}/pathtools)
endif()

## Allow this to be redirected for pycryptodome development
if(NOT DEFINED PYCRYPTODOME_INPUT)
    set( PYCRYPTODOME_INPUT ${INPUT}/pycryptodome)
endif()

set ( TELEMETRY_CONFIG_FILE ${INPUT}/telemetry/telemetry-config.json)

message("openssl"  ${OPENSSLCRYPTO_LIBRARY})

include_directories(modules)

set(REL_DIST distribution)
set(DIST "${CMAKE_BINARY_DIR}/${REL_DIST}")
set(CMAKE_INSTALL_PREFIX "${REL_DIST}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if ( PythonCoverage )
    message(With python coverage)
    SET(PYTHONPATH \$pythonzip:\$BASE_DIR/lib64/:/usr/local/lib/python3.6/dist-packages/:/usr/local/lib64/python3.6/site-packages)
    SET(PYTHON_ARGS_FOR_PROD  "-m coverage run -p" )
    SET(PYTHON_COMMAND ${PYTHON_EXECUTABLE} -m coverage run -p )
    SET(INSTALL_ADJUST "chmod 0750 \"\${SOPHOS_INSTALL}/base/lib64\"\nchown root:\${GROUP_NAME} \"\${SOPHOS_INSTALL}/base/lib64\"\nchown -R root:\${GROUP_NAME} \"\${SOPHOS_INSTALL}/base/lib64/mcsrouter\"\nchmod -R 0750 \"\${SOPHOS_INSTALL}/base/lib64/mcsrouter\"")
else()
    message( "without python coverage")
    SET(PYTHONPATH \$pythonzip:\$mcsrouterzip  )
    SET(PYTHON_ARGS_FOR_PROD  "" )
    SET(PYTHON_COMMAND ${PYTHON_EXECUTABLE})
endif()

include(cppcheck.cmake)
add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)
