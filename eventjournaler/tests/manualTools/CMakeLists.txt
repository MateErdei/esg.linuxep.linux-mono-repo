
add_executable(EventPubSub
        EventPubSub.cpp)

target_include_directories(EventPubSub PUBLIC ${PROJECT_SOURCE_DIR} ${pluginapiinclude})

target_link_libraries(EventPubSub ${zmqlib} ${pluginapilib} ${log4cpluslib} ${protobuflib})

SET_TARGET_PROPERTIES(EventPubSub
        PROPERTIES BUILD_RPATH "/opt/sophos-spl/base/lib64")
