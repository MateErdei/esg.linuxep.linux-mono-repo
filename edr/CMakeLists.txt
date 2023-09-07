cmake_minimum_required(VERSION 3.10)
project(sspl_edr_plugin)
set(CMAKE_CXX_STANDARD 17)

set(AUTOVERSION_INCLUDE ${CMAKE_SOURCE_DIR}/products/distribution/include)

option(BUILD_FUZZ_TESTS "Turn on the build of executables for fuzz test" OFF)
if (BUILD_FUZZ_TESTS )
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
    set(SANITISE_FLAGS   "-fsanitize=address -fsanitize=leak -fsanitize=undefined -fsanitize=shift-exponent -fsanitize=unreachable -fsanitize=vla-bound -fsanitize=null -fsanitize=return -fsanitize=signed-integer-overflow -fsanitize=bounds -fsanitize=alignment  -fsanitize=object-size -fsanitize=float-divide-by-zero -fsanitize=float-cast-overflow -fsanitize=returns-nonnull-attribute -fsanitize=nonnull-attribute -fsanitize=bool -fsanitize=enum -fsanitize=vptr -fsanitize=signed-integer-overflow -fsanitize-address-use-after-scope")
    set(NO_FUZZING_FLAGS "")
    set(FUZZER_COMPILER_FLAGS "" )
    set(FUZZER_LINKER_FLAGS "" )
    set(WARNING_FLAGS "-Wall -Wextra -Werror -fstack-protector-all")
endif()


set(CMAKE_SHARED_LINKER_FLAGS "-Wl,-z,relro -Wl,-z,now")
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} ${FUZZER_COMPILER_FLAGS} ${SANITISE_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${WARNING_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${SANITISE_FLAGS} ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (NOT NO_GCOV STREQUAL true AND CMAKE_BUILD_TYPE STREQUAL Debug)
    set(BUILD_WITH_GCOV  True)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -lgcov")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} --coverage")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/buildconfig")
include(SophosAddTest)
include(SophosAddLibrary)

if(NOT GCC_INPUT)
    set (GCC_INPUT "/build/input/gcc")
endif(NOT GCC_INPUT)

if(NOT DEFINED PLUGIN_NAME)
    # developers may just add a file called PLUGINNAME to get all the variables
    # set as the ./build.sh set them out.
    if ( EXISTS "${CMAKE_SOURCE_DIR}/PLUGINNAME" )
        file(READ "${CMAKE_SOURCE_DIR}/PLUGINNAME" PLUGIN_NAME_FROMFILE)
        string( STRIP ${PLUGIN_NAME_FROMFILE} PLUGIN_NAME)
    else()
    set(PLUGIN_NAME edr)
    endif()
endif()
string( TOLOWER ${PLUGIN_NAME} plugin_name)

if(NOT DEFINED PRODUCT_LINE_ID)
    set(PRODUCT_LINE_ID ServerProtectionLinux-Plugin-EDR)
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
set(FEATURE_LIST "LIVEQUERY")

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
            "    python3 -m build_scripts.artisan_fetch build-files/release-package.xml\n"
            "2. Run the full build script with no build flag:\n"
            "    ./build.sh --no-build\n"
            "After that you will be able to build using CLion")
    message( FATAL_ERROR ${ERRORMESSAGE})
endif()

if ( BUILD_FUZZ_TESTS )
    set(PROTOBUF_INPUT ${INPUT}/protobuf )
    set(Protobuf_root ${PROTOBUF_INPUT}/install64)
    set(Protobuf_INCLUDE_DIR ${Protobuf_root}/include)
    set(PROTOBUF_PROTOC_EXECUTABLE ${Protobuf_root}/bin/protoc)
    set(Protobuf_PROTOC_LIBRARY ${Protobuf_root}/lib/libprotoc.so ${Protobuf_root}/lib/libprotobuf.so )
    set(Protobuf_LITE_LIBRARY  ${Protobuf_root}/lib/libprotobuf-lite.so )
    set(Protobuf_LIBRARY ${Protobuf_root}/lib/libprotobuf.so )
    set(Protobuf_DEBUG 1)
    set(Protobuf_LD_LIBRARY_PATH ${Protobuf_root}/lib:$ENV{LD_LIBRARY_PATH} )
    set(ENV{LD_LIBRARY_PATH} ${Protobuf_LD_LIBRARY_PATH} )
    INCLUDE(FindProtobuf)
endif()

if(NOT DEFINED PLUGINAPIPATH)
    SET( PLUGINAPIPATH ${INPUT}/pluginapi)
endif()

if (NOT DEFINED LIVEQUERYPATH)
    SET( LIVEQUERYPATH ${INPUT}/livequery)
endif()

set( BOOST_INPUT ${INPUT}/boost )
set( BOOST_INCLUDE_DIR ${BOOST_INPUT} )
set( THRIFT_INPUT ${INPUT}/thrift )
set( JSONCCP_INPUT ${INPUT}/jsoncpp )
set( SQLITE_INPUT ${INPUT}/sqlite )
set( JOURNAL_INCLUDE_DIR ${INPUT}/JournalLib/include )
set( CAPNPROTO_INCLUDE_DIR ${INPUT}/capnproto/include )
set( LZMA_INCLUDE_DIR ${INPUT}/lzma/src/liblzma/api/lzma)

set( CAPNPROTO_LIBRARY_DIR ${INPUT}/capnproto/lib64)
set( LZMA_LIBRARY_DIR ${INPUT}/lzma)

set( JOURNAL_LIBRARY ${INPUT}/JournalLib/lib/libJournalLib.a)
set( LZMA_LIBRARY ${LZMA_LIBRARY_DIR}/liblzma.so)
execute_process(COMMAND chrpath -r \$ORIGIN ${LZMA_LIBRARY} COMMAND_ERROR_IS_FATAL ANY)

set(pluginapiinclude ${PLUGINAPIPATH}/include)
set(pluginapilib ${PLUGINAPIPATH}/lib64/libpluginapi.so)
set(testhelpersinclude ${PLUGINAPIPATH}/tests/include)
set(testhelperslib ${PLUGINAPIPATH}/tests/lib64/libsharedtesthelpers.so)
set(GTEST_INPUT ${INPUT}/googletest)

set(DISTRIBUTION_TOOL_DIR ${PLUGINAPIPATH}/distribution)

set(OSQUERY_BINARY ${INPUT}/osquery/opt/osquery/bin/osqueryd)
set(OSQUERY_LENSES_DIR ${INPUT}/osquery/opt/osquery/share/osquery/lenses)


set(SCHEMA_BUILDER_DIR ${CMAKE_SOURCE_DIR}/tools)

file(GLOB zmqlib ${PLUGINAPIPATH}/lib64/libzmq.so*)
execute_process(COMMAND chrpath -r \$ORIGIN ${zmqlib} COMMAND_ERROR_IS_FATAL ANY)

set(log4cpluslib ${PLUGINAPIPATH}/lib64/liblog4cplus.so)
execute_process(COMMAND chrpath -r \$ORIGIN ${log4cpluslib} COMMAND_ERROR_IS_FATAL ANY)

file(GLOB protobuflib ${PLUGINAPIPATH}/lib64/libprotobuf.so.*)
execute_process(COMMAND chrpath -r \$ORIGIN ${protobuflib} COMMAND_ERROR_IS_FATAL ANY)

file(GLOB expatlib ${PLUGINAPIPATH}/lib64/libexpat.so)
execute_process(COMMAND chrpath -r \$ORIGIN ${expatlib} COMMAND_ERROR_IS_FATAL ANY)

file(GLOB zlib ${PLUGINAPIPATH}/lib64/libzlib.so*)
execute_process(COMMAND chrpath -r \$ORIGIN ${zlib} COMMAND_ERROR_IS_FATAL ANY)


function(add_imported_library name type location include_directory)
    add_library(${name} ${type} IMPORTED)
    set_target_properties(${name} PROPERTIES IMPORTED_LOCATION ${location})
    if(${type} STREQUAL SHARED)
        set_target_properties(${name} PROPERTIES IMPORTED_NO_SONAME TRUE)
        execute_process(COMMAND chrpath -r \$ORIGIN ${location} COMMAND_ERROR_IS_FATAL ANY)
    endif()
    target_include_directories(${name} INTERFACE ${include_directory})
endfunction()

add_imported_library(sqlite_library SHARED ${SQLITE_INPUT}/libsqlite.so ${SQLITE_INPUT})
add_imported_library(thrift_library SHARED ${THRIFT_INPUT}/libthrift.so ${THRIFT_INPUT}/lib/cpp/src)
add_imported_library(logger_plugin_library SHARED ${LIVEQUERYPATH}/LoggerPlugin/libLoggerPlugin.so ${LIVEQUERYPATH}/LoggerPlugin)
add_imported_library(osquery_gen_library SHARED ${LIVEQUERYPATH}/OsquerySDK/gen-cpp-linux/libgen-cpp-linux.so ${LIVEQUERYPATH}/OsquerySDK/gen-cpp-linux)
add_imported_library(osquerysdk_library SHARED ${LIVEQUERYPATH}/OsquerySDK/libOsquerySDK.so ${LIVEQUERYPATH})
add_imported_library(openssl_ssl SHARED ${INPUT}/openssl/lib64/libssl.so.3 ${INPUT}/openssl/include)
add_imported_library(openssl_crypto SHARED ${INPUT}/openssl/lib64/libcrypto.so.3 ${INPUT}/openssl/include)

target_link_libraries(openssl_crypto INTERFACE dl pthread)
target_link_libraries(thrift_library INTERFACE ${zlib} openssl_ssl openssl_crypto)
target_link_libraries(osquery_gen_library INTERFACE thrift_library)
target_link_libraries(osquerysdk_library INTERFACE thrift_library osquery_gen_library)

set( LIVEQUERY_INCLUDE ${LIVEQUERYPATH}/common/livequery )

find_library(BOOST_THREAD_LIBRARY
        NAMES thread
        PATHS ${INPUT}/boost
        NO_DEFAULT_PATH
        )
execute_process(COMMAND chrpath -r \$ORIGIN ${BOOST_THREAD_LIBRARY} COMMAND_ERROR_IS_FATAL ANY)

find_library(JSONCPP_LIBRARY
        NAMES jsoncpp
        PATHS ${INPUT}/jsoncpp
        NO_DEFAULT_PATH
        )
execute_process(COMMAND chrpath -r \$ORIGIN ${JSONCPP_LIBRARY} COMMAND_ERROR_IS_FATAL ANY)

find_library(GLOG_LIBRARY
        NAMES glog
        PATHS ${INPUT}/glog
        NO_DEFAULT_PATH
        )
execute_process(COMMAND chrpath -r \$ORIGIN ${GLOG_LIBRARY} COMMAND_ERROR_IS_FATAL ANY)

find_library(GFLAG_LIBRARY
        NAMES gflags
        PATHS ${INPUT}/gflags
        NO_DEFAULT_PATH
        )
execute_process(COMMAND chrpath -r \$ORIGIN ${GFLAG_LIBRARY} COMMAND_ERROR_IS_FATAL ANY)

set(GTEST_INCLUDE ${GTEST_INPUT}/include)
set(GTEST_MAIN_LIBRARY ${GTEST_INPUT}/googletest_main.a)
if (NOT EXISTS ${GTEST_MAIN_LIBRARY})
    message("Library not found: ${GTEST_MAIN_LIBRARY}")
endif ()

include(cppcheck.cmake)
add_subdirectory(modules)
add_subdirectory(products)
add_subdirectory(tests)
