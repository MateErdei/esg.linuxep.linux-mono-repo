add_executable(${plugin_name}
        main.cpp ../../modules/osqueryextensions/ThreatTypes.h ../../modules/EventJournalWrapperImpl/IEventJournalReaderWrapper.h ../../modules/osqueryextensions/SophosAVDetectionTableGenerator.cpp ../../modules/osqueryextensions/SophosAVDetectionTableGenerator.h)
target_link_libraries(${plugin_name} PUBLIC pluginimpl dl ${OPENSSL_LIBRARY})

SET_TARGET_PROPERTIES( ${plugin_name}
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64:$ORIGIN/../../../base/lib64"
        )

install(TARGETS ${plugin_name}
        DESTINATION files/plugins/${PLUGIN_NAME}/bin
        PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE  GROUP_READ GROUP_EXECUTE)

