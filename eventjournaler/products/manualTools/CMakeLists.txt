
add_executable(EventPubSub
        EventPubSub.cpp)

target_include_directories(EventPubSub PUBLIC ${CMAKE_SOURCE_DIR} ${pluginapiinclude})
target_link_libraries(EventPubSub ${zmqlib} ${pluginapilib} ${log4cpluslib} ${protobuflib})

# Manual tools are never 'installed' so the BUILD_RPATH has to cover their operation on test systems.
set(MANUAL_TOOL_RPATH  "${CMAKE_BINARY_DIR}/libs:$ORIGIN:/opt/sophos-spl/plugins/eventjournaler/lib64" )

SET_TARGET_PROPERTIES(EventPubSub
        PROPERTIES
        BUILD_RPATH "${MANUAL_TOOL_RPATH}"
        )


add_executable(EventJournalWriter
        EventJournalWriter.cpp)

target_include_directories(EventJournalWriter PUBLIC ${CMAKE_SOURCE_DIR}/modules ${pluginapiinclude})
target_link_libraries(EventJournalWriter eventjournal ${pluginapilib} ${protobuflib} ${log4cpluslib} ${LZMA_LIBRARY} stdc++fs)

SET_TARGET_PROPERTIES(EventJournalWriter
        PROPERTIES
        BUILD_RPATH "${MANUAL_TOOL_RPATH}"
        )
