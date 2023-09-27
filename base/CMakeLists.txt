cmake_minimum_required(VERSION 3.12)
message("CMAKE version: ${CMAKE_VERSION}")
project(everest_base CXX)

option(test "Build all tests." ON) # Makes boolean 'test' available.
option(BUILD_FUZZ_TESTS "Turn on the build of executables for fuzz test" OFF)
option(PythonCoverage "Turn ON/OFF the instrumentation for producing python coverage results" OFF)
option(STRACE_SUPPORT "Prevent sanitize options that break strace" OFF)
option(DISABLE_SANITIZER "Disable sanitizer (libasan) for debug build" OFF)

set(CMAKE_CXX_STANDARD 17)
set(SOPHOS_SPL_USER "sophos-spl-user")
set(SOPHOS_SPL_GROUP "sophos-spl-group")
set(SOPHOS_SPL_IPC_GROUP "sophos-spl-ipc")
set(SOPHOS_SPL_LOCAL "sophos-spl-local")       #low privilege user
set(SOPHOS_SPL_UPDATESCHEDULER "sophos-spl-updatescheduler")  #user which updatescheduler runs as

execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE)

if (BUILD_FUZZ_TESTS)
    set(SANITISE_FLAGS "")
    set(SANITIZE_EXE_FLAGS "")
    set(FUZZER_COMPILER_FLAGS "-fsanitize=address,fuzzer-no-link")
    set(FUZZER_LINKER_FLAGS "-fsanitize=address,fuzzer")
    set(NO_FUZZING_FLAGS "-fno-sanitize=fuzzer")
    set(WARNING_FLAGS "")
elseif (DISABLE_SANITIZER)
    set(SANITISE_FLAGS "")
    set(NO_FUZZING_FLAGS "")
    set(FUZZER_COMPILER_FLAGS "")
    set(FUZZER_LINKER_FLAGS "")
    set(WARNING_FLAGS "-Wall -Wextra -Werror -fstack-protector-all")
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
    set (GCC_INPUT "/build/redist/gcc")
endif(NOT GCC_INPUT)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif(NOT CMAKE_BUILD_TYPE)

# PRODUCT_NAME
if(NOT DEFINED PRODUCT_NAME)
    if (DEFINED ENV{PRODUCT_NAME})
        set(PRODUCT_NAME "$ENV{PRODUCT_NAME}")
    else()
        set(PRODUCT_NAME "SPL-Base-Component")
    endif()
endif()

# PRODUCT_LINE_ID
if(NOT DEFINED PRODUCT_LINE_ID)
    if (DEFINED ENV{PRODUCT_LINE_ID})
        set(PRODUCT_LINE_ID "$ENV{PRODUCT_LINE_ID}")
    else()
        set(PRODUCT_LINE_ID "ServerProtectionLinux-Base-component")
    endif()
endif()

set(RA_PLUGIN_NAME responseactions)

set(RA_PRODUCT_LINE_ID ServerProtectionLinux-Plugin-${RA_PLUGIN_NAME})
set(RA_PRODUCT_NAME SPL-Response-Actions-Plugin)

# DEFAULT_HOME_FOLDER
if(NOT DEFINED DEFAULT_HOME_FOLDER)
    if (DEFINED ENV{DEFAULT_HOME_FOLDER})
        set(DEFAULT_HOME_FOLDER "$ENV{DEFAULT_HOME_FOLDER}")
    else()
        set(DEFAULT_HOME_FOLDER "sspl-base")
    endif()
endif()

set(DISTRIBUTION_TOOL_DIR ${CMAKE_SOURCE_DIR}/products/distribution)

if(NOT DEFINED UPDATESCHEDULER_PRETTY_NAME)
    set(UPDATESCHEDULER_PRETTY_NAME "Update Scheduler")
endif()

#Specify the feature list of the form "feature1, feature2, feature3"
#Set "" for no features when including in a component suite
set(FEATURE_LIST "CORE")
set(RA_FEATURE_LIST "MDR")

set(BASE_DIST_PREFIX "base")
set(RA_DIST_PREFIX "ra")

find_program(VERSIONING_CLIENT "versioning_client"
        PATHS ${CMAKE_SOURCE_DIR}/../tapvenv/bin
        REQUIRED)

if (DEFINED ENV{VERSION_OVERRIDE})
    set(PRODUCT_VERSION "$ENV{VERSION_OVERRIDE}")
else()
    execute_process(
            COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/products/distribution/getReleasePackageVersion.py ${CMAKE_SOURCE_DIR}/build/release-package.xml
            OUTPUT_VARIABLE RELEASE_PACKAGE_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
            COMMAND ${VERSIONING_CLIENT} --component sspl_base --version_number ${RELEASE_PACKAGE_VERSION}
            OUTPUT_VARIABLE PRODUCT_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(FATAL_ERROR \"Failed to generate version from base: ${STATUS}\")
    endif()
endif()

if (DEFINED ENV{VERSION_OVERRIDE})
    set(RA_PRODUCT_VERSION "$ENV{VERSION_OVERRIDE}")
else()
    execute_process(
            COMMAND ${VERSIONING_CLIENT} --component responseactions --version_number 1.0.1
            OUTPUT_VARIABLE RA_PRODUCT_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(FATAL_ERROR \"Failed to generate version from base: ${STATUS}\")
    endif()
endif()

find_package(PkgConfig)
FIND_PACKAGE ( Threads REQUIRED )

# /build/input is where inputs are fetched from artifactory
if(NOT EXISTS /build/input)
    message("Fetching build inputs because /build/input does not exist.")
    execute_process(
            COMMAND bash tap_fetch.sh
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# /build/redist is currently named INPUT in our cmake, it is where inputs are unpacked to
if(NOT DEFINED INPUT)
    set( INPUT /build/redist )
endif()

execute_process(
        COMMAND bash unpack_build_inputs.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE unpack_result)

if (NOT ${unpack_result} EQUAL 0)
    message(FATAL_ERROR "Unpacking build dependencies failed")
endif()

## Find 3rd party libraries
set( BOOST_INPUT ${INPUT}/boost )
set( CURL_INPUT ${INPUT}/curl )
set( OPENSSL_INPUT ${INPUT}/openssl )
set( EXPAT_INPUT ${INPUT}/expat )
set( LOG4CPLUS_INPUT ${INPUT}/log4cplus )
set( GSL_INCLUDE ${INPUT}/gsl/include )
set( BOOST_INCLUDE ${BOOST_INPUT}/include )
set( GTEST_INPUT ${INPUT}/googletest )
set(LIBXCRYPT_INPUT ${INPUT}/libxcrypt)

if(NOT DEFINED VERSIG_INPUT)
    set( VERSIG_INPUT ${INPUT}/versig)
endif()

function(add_imported_library name type no_soname location include_directory)
    add_library(${name} ${type} IMPORTED)
    if (${type} STREQUAL SHARED)
        execute_process(COMMAND chrpath -r \$ORIGIN ${location} COMMAND_ERROR_IS_FATAL ANY)
    endif ()
    set_target_properties(${name} PROPERTIES IMPORTED_LOCATION ${location})
    set_target_properties(${name} PROPERTIES IMPORTED_NO_SONAME ${no_soname})
    target_include_directories(${name} INTERFACE ${include_directory})
endfunction()

set(SOPHLIB_INCLUDE ${INPUT}/sophlib/include )

function(add_sophlib_library name)
    # Linked statically to avoid name clashes with our libraries
    add_imported_library(sophlib_${name} STATIC FALSE ${INPUT}/sophlib/lib64/${name}/lib${name}.a ${SOPHLIB_INCLUDE})
endfunction()

function(monorepo_library name location include_directory)
    add_imported_library(${name} SHARED TRUE ${location}.so ${include_directory})
    add_imported_library(${name}_static STATIC FALSE ${location}.a ${include_directory})
endfunction()

monorepo_library(pugixml ${INPUT}/pugixml/libpugixml ${INPUT}/pugixml)
monorepo_library(bzip2 ${INPUT}/bzip2/libbzip2 ${INPUT}/bzip2)
monorepo_library(lzma ${INPUT}/lzma/liblzma ${INPUT}/lzma/src/liblzma/api)
monorepo_library(minizip ${INPUT}/minizip/minizip/libminizip ${INPUT}/minizip)
monorepo_library(zlib ${INPUT}/zlib/libzlib ${INPUT}/zlib)
monorepo_library(curl_library ${CURL_INPUT}/libcurl ${CURL_INPUT}/include)

# Extra search path to allow #include <curl.h>
target_include_directories(curl_library INTERFACE ${CURL_INPUT}/include/curl)
target_include_directories(curl_library_static INTERFACE ${CURL_INPUT}/include/curl)

add_imported_library(openssl_ssl SHARED FALSE ${OPENSSL_INPUT}/lib64/libssl.so.3 ${OPENSSL_INPUT}/include)
add_imported_library(openssl_ssl_static STATIC FALSE ${OPENSSL_INPUT}/lib64/libssl.a ${OPENSSL_INPUT}/include)
add_imported_library(openssl_crypto SHARED FALSE ${OPENSSL_INPUT}/lib64/libcrypto.so.3 ${OPENSSL_INPUT}/include)
add_imported_library(openssl_crypto_static STATIC FALSE ${OPENSSL_INPUT}/lib64/libcrypto.a ${OPENSSL_INPUT}/include)

add_sophlib_library(crypto)
add_sophlib_library(file)
add_sophlib_library(http)
add_sophlib_library(hostname)
add_sophlib_library(logging)
add_sophlib_library(raii)
add_sophlib_library(rapidjson)
add_sophlib_library(sdds3)
add_sophlib_library(string)
add_sophlib_library(time)
add_sophlib_library(zip)
add_imported_library(sophlib_sdds3_delta STATIC FALSE ${INPUT}/sophlib/lib64/sdds3/delta/libdelta.a ${INPUT}/sophlib/include)

# Define dependencies between imported libraries (based on Bazel dependencies in monorepo)
target_link_libraries(minizip INTERFACE bzip2 lzma openssl_crypto zlib)
target_link_libraries(minizip_static INTERFACE bzip2_static lzma_static openssl_crypto_static zlib_static)
target_link_libraries(sophlib_file INTERFACE sophlib_raii)
target_link_libraries(sophlib_hostname INTERFACE sophlib_string)
target_link_libraries(sophlib_rapidjson INTERFACE
        sophlib_logging
        )
target_link_libraries(sophlib_sdds3 INTERFACE
        pugixml_static
        sophlib_crypto
        sophlib_file
        sophlib_http
        sophlib_rapidjson
        sophlib_sdds3_delta
        sophlib_string
        sophlib_time
        sophlib_zip
        )
target_link_libraries(sophlib_zip INTERFACE minizip_static sophlib_logging)
target_link_libraries(curl_library INTERFACE openssl_ssl zlib)
target_link_libraries(curl_library_static INTERFACE openssl_ssl_static zlib_static)
target_link_libraries(openssl_crypto INTERFACE dl pthread)
target_link_libraries(openssl_crypto_static INTERFACE dl pthread)
target_link_libraries(openssl_ssl INTERFACE openssl_crypto)
target_link_libraries(openssl_ssl_static INTERFACE openssl_crypto_static)

set(NLOHMANN_JSON_INCLUDE_DIR ${INPUT}/nlohmann-json/single_include/nlohmann)

set( BOOST_INCLUDE_DIR ${BOOST_INPUT} )

set( PROTOBUF_INPUT ${INPUT}/protobuf )
set(Protobuf_root ${PROTOBUF_INPUT}/install64)
set(Protobuf_INCLUDE_DIR ${Protobuf_root}/include)
set(PROTOBUF_PROTOC_EXECUTABLE ${Protobuf_root}/bin/protoc)
set(Protobuf_PROTOC_LIBRARY ${Protobuf_root}/lib/libprotoc.so ${Protobuf_root}/lib/libprotobuf.so )
set(Protobuf_LITE_LIBRARY  ${Protobuf_root}/lib/libprotobuf-lite.so )
set(Protobuf_LIBRARY ${Protobuf_root}/lib/libprotobuf.so )
set(Protobuf_DEBUG 1)
set(Protobuf_LD_LIBRARY_PATH ${Protobuf_root}/lib:${GCC_INPUT}/lib64:$ENV{LD_LIBRARY_PATH} )
set( ENV{LD_LIBRARY_PATH} ${Protobuf_LD_LIBRARY_PATH} )
INCLUDE(FindProtobuf)

#FIND_PACKAGE(Protobuf REQUIRED )
configure_file(build/protoc.sh.in protoc.sh)

set( ZEROMQ_INPUT ${INPUT}/zeromq )
## use pkg-config to get hints for 0mq locations
#pkg_check_modules(PC_ZeroMQ QUIET zmq)

## load in pkg-config support
## use the hint from above to find where 'zmq.hpp' is located
find_path(ZeroMQ_INCLUDE_DIR
        NAMES zmq.h
        PATHS ${ZEROMQ_INPUT}
        )

## use the hint from above to find the location of libzmq
find_library(ZeroMQ_LIBRARY
        NAMES zmq
        PATHS ${ZEROMQ_INPUT}
        )

execute_process(
        COMMAND chrpath -r \$ORIGIN ${ZeroMQ_LIBRARY}
        COMMAND_ERROR_IS_FATAL ANY)

find_library(EXPAT_LIBRARY
        NAMES expat
        PATHS ${EXPAT_INPUT}
        NO_DEFAULT_PATH
        )
execute_process(COMMAND chrpath -r \$ORIGIN ${EXPAT_LIBRARY} COMMAND_ERROR_IS_FATAL ANY)

execute_process(COMMAND find ${BOOST_INPUT} -name *.so -exec chrpath -r $ORIGIN {} \; COMMAND_ERROR_IS_FATAL ANY)

find_library(BOOST_LOCALE
        NAMES locale
        PATHS ${BOOST_INPUT}/
        NO_DEFAULT_PATH
        )

set(LOG4CPLUS_INCLUDE_DIR ${LOG4CPLUS_INPUT}/include)
find_library(LOG4CPLUS_LIBRARY
        NAMES log4cplus
        PATHS ${LOG4CPLUS_INPUT}
        NO_DEFAULT_PATH
        NO_CMAKE_FIND_ROOT_PATH
        )
execute_process(COMMAND chrpath -r \$ORIGIN ${LOG4CPLUS_LIBRARY} COMMAND_ERROR_IS_FATAL ANY)

# ensure valid log4cplus to work with fork
file(STRINGS ${LOG4CPLUS_LIBRARY} log4cplusStrings)
string(FIND "${log4cplusStrings}" "implicit initialization is turned off" DisabledImplicitInitialization)
if (${DisabledImplicitInitialization} EQUAL -1)
    string(APPEND ERRORMESSAGE
            "Log4cplus (${LOG4CPLUS_LIBRARY}) needs to be built with --disable-implicit-initialization to work in the presence of fork. \n"
            "Make sure you have fetched the inputs and build with build.sh at least once\n"
            "1. Run the full build script with no build flag:\n"
            "    ./build.sh --setup --no-build\n")
    message(FATAL_ERROR ${ERRORMESSAGE})
endif ()

set(GTEST_INCLUDE ${GTEST_INPUT}/include)
set(GTEST_MAIN_LIBRARY ${GTEST_INPUT}/googletest_main.a)
if (NOT EXISTS ${GTEST_MAIN_LIBRARY})
    message("Library not found: ${GTEST_MAIN_LIBRARY}")
endif ()

## Allow this to be redirected for python development
if (NOT DEFINED PYTHON_INPUT)
    set(PYTHON_INPUT ${INPUT}/python)
endif ()
set(PYTHON_ZIP python311.zip)
set(PYTHON_VERSION python3.11)
set(PYTHON_LIB_DIR "${PYTHON_INPUT}/lib/${PYTHON_VERSION}")
find_library(PYTHON_LIBRARY
        NAMES "${PYTHON_VERSION}"
        PATHS ${PYTHON_INPUT}/lib
        )
file(GLOB PYTHON_LIBRARIES "${PYTHON_LIBRARY}.*")

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
    set( PYCRYPTODOME_INPUT ${INPUT}/pypi/)
endif()

# Telemetry config is different for dev vs prod builds
set(TELEMETRY_CONFIG_SOURCE "${CMAKE_SOURCE_DIR}/build/dev-telemetry-config.json")
if (DEFINED ENV{SOURCE_CODE_BRANCH})
    if ($ENV{SOURCE_CODE_BRANCH} MATCHES "^release/")
        set(TELEMETRY_CONFIG_SOURCE "${CMAKE_SOURCE_DIR}/build/prod-telemetry-config.json")
    endif()
endif()
set(TELEMETRY_CONFIG_FILE ${INPUT}/telemetry/telemetry-config.json)
configure_file(${TELEMETRY_CONFIG_SOURCE} ${TELEMETRY_CONFIG_FILE} COPYONLY)
message("Telemetry config: ${TELEMETRY_CONFIG_SOURCE} -> ${TELEMETRY_CONFIG_FILE}")

set(AUTOVERSION_INCLUDE ${CMAKE_SOURCE_DIR}/products/distribution/include)

include_directories(modules)

set(REL_DIST distribution)
set(DIST "${CMAKE_BINARY_DIR}/${REL_DIST}")
set(OUTPUT "${CMAKE_SOURCE_DIR}/output")
set(CMAKE_INSTALL_PREFIX "${REL_DIST}")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# MCS Python settings, used in registerCentral.in.sh, mcsrouter.in.sh and python unit tests.
if (PythonCoverage)
    message(With python coverage)
    set(PYTHONPATH /usr/lib/python3.8/:\$BASE_DIR/lib64/) #Python path must include base MCS code to run mcsrouter etc.
    set(PYTHON_ARGS_FOR_PROD  "-m coverage run -p") # args passed to product python
    set(PYTHON_COMMAND ${PYTHON_EXECUTABLE} -m coverage run -p) # for python unit tests
    set(INSTALL_ADJUST "chmod 0750 \"\${SOPHOS_INSTALL}/base/lib64\"\nchown root:\${GROUP_NAME} \"\${SOPHOS_INSTALL}/base/lib64\"\nchown -R root:\${GROUP_NAME} \"\${SOPHOS_INSTALL}/base/lib64/mcsrouter\"\nchmod -R 0750 \"\${SOPHOS_INSTALL}/base/lib64/mcsrouter\"")
    set(PRODUCT_PYTHON_EXECUTABLE "python3")
else()
    message("without python coverage")
    set(PYTHONPATH \$pythonzip:\$mcsrouterzip)
    set(PYTHON_ARGS_FOR_PROD  "")
    set(PYTHON_COMMAND ${PYTHON_EXECUTABLE})
    set(PRODUCT_PYTHON_EXECUTABLE "\$BASE_DIR/bin/python3")
    set(PRODUCT_PYTHONHOME "\$INST_DIR/base/")
    set(PRODUCT_PYTHON_LD_LIBRARY_PATH "\$INST_DIR/base/lib:\$INST_DIR/base/lib64")
endif()

# set as var to pass around as extra install locations for libs needed by mcs.
set(THININSTALLER_DEPS_DIR_LIB64 ${CMAKE_BINARY_DIR}/products/ThininstallerDependencies/mcslibrary/lib64)
set(THININSTALLER_DEPS_DIR_BIN ${CMAKE_BINARY_DIR}/products/ThininstallerDependencies/mcslibrary/bin)

# Set extra files and dirs to delete when we run clean target
set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        ${CMAKE_BINARY_DIR}/libs
        ${CMAKE_BINARY_DIR}/PostInstall
        ${CMAKE_BINARY_DIR}/protoc.sh
        ${CMAKE_BINARY_DIR}/SystemProductTestOutput
        ${CMAKE_BINARY_DIR}/Testing
        )

include_directories(
        ${CMAKE_SOURCE_DIR} # To be able to include paths starting with "tests"
        ${CMAKE_BINARY_DIR}/modules # To be able to include autogenerated headers
)

include(cppcheck.cmake)
add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)

# Must come last
add_subdirectory(PostInstall)