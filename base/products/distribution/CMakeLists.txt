
add_custom_command(OUTPUT ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
        COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/generateSDDSImportAndManifestDat.py ${DIST}
        )

add_custom_target(
        dist
        DEPENDS ${DIST}/SDDS-Import.xml ${DIST}/manifest.dat
)

set(third_party_libs)

function(add_third_part_library LIB LIBRARY_PATH)
    set(FOUND_LIB ${LIB}-NOTFOUND)
    FIND_LIBRARY(FOUND_LIB
            NAMES ${LIB}
            PATHS ${LIBRARY_PATH}
            NO_DEFAULT_PATH
            )
    GET_FILENAME_COMPONENT(FOUND_LIB_ABSOLUTE ${FOUND_LIB} REALPATH)
    set(third_party_libs ${third_party_libs} ${FOUND_LIB_ABSOLUTE} PARENT_SCOPE)

endfunction(add_third_part_library)

foreach (LIB boost_atomic boost_chrono boost_date_time boost_filesystem boost_regex boost_system boost_thread)
    add_third_part_library(${LIB} ${BOOST_INPUT}/lib64)
endforeach(LIB)

add_third_part_library(curl ${CURL_INPUT}/lib64)

add_third_part_library(expat ${EXPAT_INPUT}/lib64)

foreach (LIB crypto ssl)
    add_third_part_library(${LIB} ${OPENSSL_INPUT}/lib64)
endforeach(LIB)

foreach (LIB SUL retailer_lib xmlcpp aesclib saucrypto)
    add_third_part_library(${LIB} ${SUL_LIBRARY_DIR})
endforeach(LIB)

add_third_part_library(protobuf ${Protobuf_root}/lib)

install(FILES ${third_party_libs}
        DESTINATION files/base/lib64)
