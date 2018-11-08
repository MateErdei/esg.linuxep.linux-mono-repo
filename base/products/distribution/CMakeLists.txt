

configure_file(PRODUCT_NAME.in PRODUCT_NAME @ONLY )
configure_file(RIGID_NAME.in RIGID_NAME @ONLY )
configure_file(DEFAULT_HOME_FOLDER.in DEFAULT_HOME_FOLDER @ONLY )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libstdc++.so
        OUTPUT_VARIABLE libstdcpp_path)
string(STRIP "${libstdcpp_path}" libstdcpp_path )

execute_process(COMMAND bash ${CMAKE_SOURCE_DIR}/build/gcc-print-file-name.sh libgcc_s.so.1
        OUTPUT_VARIABLE libgcc_s_path)
string(STRIP "${libgcc_s_path}" libgcc_s_path )

set(third_party_libs ${libstdcpp_path} ${libgcc_s_path})

add_custom_command(OUTPUT ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generateSDDSImportAndManifestDat.py ${DIST}
        )

add_custom_target(
        dist
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generateSDDSImportAndManifestDat.py ${DIST}
)


add_custom_target(
        forcedist
        COMMAND rm -f ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generateSDDSImportAndManifestDat.py ${DIST}
)
#
#install(CODE "
#        execute_process(
#            COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generateSDDSImportAndManifestDat.py ${DIST}
#        )
#")

function(add_third_party_library LIB LIBRARY_PATH)
    FindLibraryFullPath(FOUND_LIB_ABSOLUTE ${LIB} ${LIBRARY_PATH})
    set(third_party_libs ${third_party_libs} ${FOUND_LIB_ABSOLUTE} PARENT_SCOPE)

endfunction(add_third_party_library)

foreach (LIB boost_atomic boost_chrono boost_date_time boost_filesystem boost_regex boost_system boost_thread)
    add_third_party_library(${LIB} ${BOOST_INPUT}/lib64)
endforeach(LIB)

add_third_party_library(curl ${CURL_INPUT}/lib64)

add_third_party_library(expat ${EXPAT_INPUT}/lib64)

add_third_party_library(zmq ${ZEROMQ_INPUT}/lib)

foreach (LIB crypto ssl)
    add_third_party_library(${LIB} ${OPENSSL_INPUT}/lib64)
endforeach(LIB)

foreach (LIB SUL retailer_lib xmlcpp aesclib saucrypto)
    add_third_party_library(${LIB} ${SUL_LIBRARY_DIR})
endforeach(LIB)

add_third_party_library(protobuf ${Protobuf_root}/lib)
add_third_party_library(log4cplus ${LOG4CPLUS_INPUT}/lib)

install(FILES ${third_party_libs}
        DESTINATION files/base/lib64)

install(FILES ${VERSIG_INPUT}/bin64/versig
        DESTINATION files/base/update)

add_custom_target(
        copy_libs
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/libs
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/copy_libs.py ${CMAKE_BINARY_DIR}/libs ${third_party_libs}
)

