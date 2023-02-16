cmake_minimum_required(VERSION 3.5)
project(thininstaller CXX)

set(CMAKE_CXX_STANDARD 17)

# This is called by the buildLinux.sh script which should have setup all dependencies in ./redist

execute_process(
        COMMAND bash "-c" "ldd --version | grep 'ldd (.*)' | rev | cut -d ' ' -f 1 | rev"
        OUTPUT_VARIABLE BUILD_SYSTEM_LIBC_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
        COMMAND bash "-c" "grep 'package name=\"sspl-thininstaller\"' release-package.xml | sed -e s/.*version=\\\"// -e s/\\\"\\>//"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE VERSION_REPLACEMENT_STRING
        OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/build-scripts/computeAutoVersion.py ${CMAKE_CURRENT_SOURCE_DIR} "${VERSION_REPLACEMENT_STRING}"  CIJenkinsfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE PRODUCT_VERSION_REPLACEMENT_STRING
        OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build-scripts/gcc-print-file-name.sh libatomic.so.1
        OUTPUT_VARIABLE libatomic_path)
string(STRIP "${libatomic_path}" libatomic_path )


configure_file(${CMAKE_SOURCE_DIR}/installer_header.in.sh ${CMAKE_SOURCE_DIR}/installer_header.sh @ONLY)

if(NOT DEFINED REDIST)
    set( REDIST /build/redist )
endif()

if(NOT EXISTS ${REDIST})
    string( APPEND ERRORMESSAGE
            "To build using CLion you first need to:\n"
            "1. Fetch inputs in source folder by running:\n"
            "    python3 -m build_scripts.artisan_fetch release-package.xml\n"
            "2. Run the full build script with no build flag:\n"
            "    ./buildLinux.sh --no-build\n"
            "After that you will be able to build using CLion")
    message( FATAL_ERROR ${ERRORMESSAGE})
endif()

#SUL Library
set(SUL_INPUT ${REDIST}/SUL)
set(BOOST_INPUT ${REDIST}/boost)
set(EXPAT_INPUT ${REDIST}/expat)
set(OPENSSL_INPUT ${REDIST}/openssl)
set(ZLIB_INPUT ${REDIST}/zlib)
set(VERSIG_INPUT ${REDIST}/versig)

#curl Library
set(CURL_INPUT ${REDIST}/curl)
set(CURL_LIBRARY_DIR ${CURL_INPUT}/lib64)
set(CURL_LIBRARY -L${CURL_LIBRARY_DIR}  -lcurl)

#For central Registration
set(MCS_LIBRARY_INPUT ${REDIST}/mcslibrary)
set(MCS_LIBRARY_INCLUDE ${MCS_LIBRARY_INPUT}/include)
set(MCS_LIBRARY_DIR ${MCS_LIBRARY_INPUT}/lib64)
set(MCS_LIBRARY -L${MCS_LIBRARY_DIR})
file(GLOB zmqlib ${MCS_LIBRARY_DIR}/libzmq.so.*)
file(GLOB log4cpluslib ${MCS_LIBRARY_DIR}/liblog4cplus-*.so.*)
file(GLOB protobuflib ${MCS_LIBRARY_DIR}/libprotobuf.so.*)

#Pluginapi (for common code)
set(PLUGINAPI_LIBRARY_INPUT ${REDIST}/pluginapi)
set(PLUGINAPI_LIBRARY_INCLUDE ${PLUGINAPI_LIBRARY_INPUT}/include)

#Include dirs
include_directories(BEFORE ${CURL_INPUT}/include64 ${MCS_LIBRARY_INCLUDE} ${PLUGINAPI_LIBRARY_INCLUDE})

# Set flags
set(CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,\$ORIGIN/../lib64)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wall -fstack-protector-all")

add_executable(installer installer.cpp)

target_include_directories(installer PUBLIC ${MCS_LIBRARY_DIR}  ${PLUGINAPI_LIBRARY_INCLUDE} )
target_link_libraries(installer PUBLIC  ${CURL_LIBRARY} ${MCS_LIBRARY}
        libcmcsrouterapi.so
        ${zmqlib}
        ${log4cpluslib}
        ${protobuflib}
        ${libatomic_path}
        )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build-scripts/gcc-print-file-name.sh libstdc++.so
        OUTPUT_VARIABLE libstdcpp_path)
string(STRIP "${libstdcpp_path}" libstdcpp_path )

get_filename_component(libstcpp_name "${libstdcpp_path}" NAME)

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build-scripts/gcc-print-file-name.sh libgcc_s.so.1
        OUTPUT_VARIABLE libgcc_s_path)
string(STRIP "${libgcc_s_path}" libgcc_s_path )


if (NOT DEFINED OUTPUT)
    set(OUTPUT  ${CMAKE_BINARY_DIR}/output)
endif()

if (NOT DEFINED INSTALLERDIR)
    set(INSTALLERDIR ${CMAKE_BINARY_DIR}/installer)
endif()

set(bin_dir ${INSTALLERDIR}/bin)
set(libs_dir ${INSTALLERDIR}/lib64)

SET_TARGET_PROPERTIES( installer
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS installer
        DESTINATION ${bin_dir})

add_custom_target(
        copyInstaller
        COMMAND mkdir -p "${bin_dir}"
        COMMAND cp -a ${VERSIG_INPUT}/bin64/versig ${bin_dir}/
        COMMAND cp -a ${MCS_LIBRARY_INPUT}/bin/SulDownloader* ${bin_dir}/
        COMMAND strip ${bin_dir}/*
        COMMAND chmod 700 ${bin_dir}/*
        COMMAND mkdir -p "${libs_dir}"
        COMMAND cp -a ${SUL_INPUT}/lib64/*.so* ${libs_dir}/
        COMMAND cp -a ${CURL_INPUT}/lib64/*.so* ${libs_dir}/
        COMMAND cp -a ${BOOST_INPUT}/lib64/*.so* ${libs_dir}/
        COMMAND cp -a ${OPENSSL_INPUT}/lib64/*.so* ${libs_dir}/
        COMMAND cp -a ${EXPAT_INPUT}/*.so* ${libs_dir}/
        COMMAND cp -a ${ZLIB_INPUT}/*.so* ${libs_dir}/
        COMMAND cp -a ${libstdcpp_path} ${libs_dir}/
        COMMAND cp -a ${libatomic_path} ${libs_dir}/
        COMMAND cp -a ${libgcc_s_path} ${libs_dir}/
        COMMAND cp -a ${MCS_LIBRARY_INPUT}/lib64/*.so* ${libs_dir}/
        COMMAND strip ${libs_dir}/*
        COMMAND chmod 700 ${libs_dir}/*
        COMMAND ln -snf ${libstcpp_name} ${libs_dir}/libstdc++.so.6
        COMMAND cp ${CMAKE_SOURCE_DIR}/rootca.crt ${INSTALLERDIR}/
        COMMAND cp ${CMAKE_SOURCE_DIR}/rootca384.crt ${INSTALLERDIR}/
        COMMAND cp ${CMAKE_SOURCE_DIR}/ps_rootca.crt ${INSTALLERDIR}/
        COMMAND cp ${CMAKE_SOURCE_DIR}/certs/mcs_rootca.crt ${INSTALLERDIR}/
        COMMAND cp ${CMAKE_SOURCE_DIR}/sdds_https_rootca.crt ${INSTALLERDIR}/
        COMMAND ldconfig -n ${libs_dir}/
)

