
add_executable(EventPubSub
        EventPubSub.cpp)

add_executable(EventJournalWriter
        EventJournalWriter.cpp)

target_include_directories(EventPubSub PUBLIC ../.. ${pluginapiinclude})
target_include_directories(EventJournalWriter PUBLIC ${CMAKE_SOURCE_DIR}/modules ${pluginapiinclude})

target_link_libraries(EventPubSub ${zmqlib} ${pluginapilib} ${log4cpluslib} ${protobuflib})
target_link_libraries(EventJournalWriter eventjournal ${pluginapilib} ${protobuflib} ${log4cpluslib} ${LZMA_LIBRARY} stdc++fs)

SET_TARGET_PROPERTIES(EventPubSub
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/base/lib64;/opt/sophos-spl/plugins/eventjournaler/lib64;${CMAKE_BINARY_DIR}/libs")

SET_TARGET_PROPERTIES(EventJournalWriter
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/base/lib64;/opt/sophos-spl/plugins/eventjournaler/lib64;${CMAKE_BINARY_DIR}/libs")
