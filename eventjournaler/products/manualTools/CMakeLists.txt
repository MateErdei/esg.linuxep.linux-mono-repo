
add_executable(EventPubSub
        EventPubSub.cpp)

target_include_directories(EventPubSub PUBLIC ${CMAKE_SOURCE_DIR} ${pluginapiinclude})
target_link_libraries(EventPubSub ${zmqlib} ${pluginapilib} ${log4cpluslib} ${protobuflib})

SET_TARGET_PROPERTIES(EventPubSub
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )


add_executable(EventJournalWriter
        EventJournalWriter.cpp)

target_include_directories(EventJournalWriter PUBLIC ${CMAKE_SOURCE_DIR}/modules ${pluginapiinclude})
target_link_libraries(EventJournalWriter eventjournal ${pluginapilib} ${protobuflib} ${log4cpluslib} ${LZMA_LIBRARY} stdc++fs)

SET_TARGET_PROPERTIES(EventJournalWriter
        PROPERTIES
        INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )
