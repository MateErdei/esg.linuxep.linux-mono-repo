
configure_file(PRODUCT_NAME.in PRODUCT_NAME @ONLY )
configure_file(RIGID_NAME.in RIGID_NAME @ONLY )
configure_file(DEFAULT_HOME_FOLDER.in DEFAULT_HOME_FOLDER @ONLY )
configure_file(FEATURE_LIST.in FEATURE_LIST @ONLY )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libstdc++.so
        OUTPUT_VARIABLE libstdcpp_path)
string(STRIP "${libstdcpp_path}" libstdcpp_path )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libgcc_s.so.1
        OUTPUT_VARIABLE libgcc_s_path)
string(STRIP "${libgcc_s_path}" libgcc_s_path )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libatomic.so.1
        OUTPUT_VARIABLE libatomic_path)
string(STRIP "${libatomic_path}" libatomic_path )

set(third_party_libs ${libstdcpp_path} ${libgcc_s_path} ${libatomic_path})
set(third_party_libs_ra ${libstdcpp_path} ${libgcc_s_path} ${libatomic_path})

INCLUDE(FindLibraryFullPath)

function(add_third_party_library LIB LIBRARY_PATH)
    FindLibraryFullPath(FOUND_LIB_ABSOLUTE ${LIB} ${LIBRARY_PATH})
    set(third_party_libs ${third_party_libs} ${FOUND_LIB_ABSOLUTE} PARENT_SCOPE)

    # Third party libraries also required by RA plugin
    foreach(RA_LIB expat zmq protobuf log4cplus system)
        if (${LIB} STREQUAL ${RA_LIB})
            set(third_party_libs_ra ${third_party_libs_ra} ${FOUND_LIB_ABSOLUTE} PARENT_SCOPE)
        endif()
    endforeach()
endfunction(add_third_party_library)


function(add_third_party_library_ra_only LIB LIBRARY_PATH)
    FindLibraryFullPath(FOUND_LIB_ABSOLUTE ${LIB} ${LIBRARY_PATH})
    set(third_party_libs_ra ${third_party_libs_ra} ${FOUND_LIB_ABSOLUTE} PARENT_SCOPE)
endfunction(add_third_party_library_ra_only)
add_third_party_library_ra_only(locale ${BOOST_INPUT})
add_third_party_library_ra_only(system ${BOOST_INPUT})

add_third_party_library(expat ${EXPAT_INPUT})
add_third_party_library(zmq ${ZEROMQ_INPUT})

add_third_party_library(protobuf ${Protobuf_root}/lib)
add_third_party_library(log4cplus ${LOG4CPLUS_INPUT})

install(FILES ${third_party_libs}
        DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

install(FILES ${third_party_libs_ra}
        DESTINATION ${RA_DIST_PREFIX}/files/plugins/${RA_PLUGIN_NAME}/lib64)

install(IMPORTED_RUNTIME_ARTIFACTS
        bzip2
        curl_library
        lzma
        minizip
        openssl_ssl
        openssl_crypto
        zlib
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

install(IMPORTED_RUNTIME_ARTIFACTS
        bzip2
        curl_library
        lzma
        minizip
        openssl_ssl
        openssl_crypto
        zlib
        DESTINATION ${RA_DIST_PREFIX}/files/plugins/${RA_PLUGIN_NAME}/lib64)

install(FILES ${VERSIG_INPUT}/bin64/versig
        DESTINATION ${BASE_DIST_PREFIX}/files/base/update)

# libasan and libubsan are only used during debug builds and needed for unit tests to run but we do not want to add
# them to the shipped third party library list, so create a separate list of build time only libs
execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libasan.so
        OUTPUT_VARIABLE libasan_path)
string(STRIP "${libasan_path}" libasan_path)
execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libubsan.so
        OUTPUT_VARIABLE libubsan_path)
string(STRIP "${libubsan_path}" libubsan_path )
set(build_time_only_libs ${libasan_path} ${libubsan_path})

# Put all needed libs into <build dir>/libs so that unit tests can link against them without setting LD_LIBARY_PATH.
add_custom_target(
        copy_libs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/libs
        COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/copy_libs.py ${CMAKE_BINARY_DIR}/libs ${third_party_libs} ${build_time_only_libs}
)

configure_file(VERSION.ini.in VERSION.ini @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VERSION.ini
        DESTINATION ${BASE_DIST_PREFIX}/files/base/)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/VERSION.ini
        DESTINATION ${BASE_DIST_PREFIX})

install(FILES datafeed-config-scheduled_query.json
        DESTINATION ${BASE_DIST_PREFIX}/files/base/etc/)

