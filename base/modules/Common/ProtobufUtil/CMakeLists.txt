add_library(protobufutilobject OBJECT
        MessageUtility.cpp
        MessageUtility.h
        MessageException.h
        )


target_include_directories(protobufutilobject SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR})

add_library(protobufutil SHARED $<TARGET_OBJECTS:protobufutilobject>)
target_link_libraries(protobufutil PUBLIC ${Protobuf_LIBRARIES})

SET_TARGET_PROPERTIES(protobufutil
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS protobufutil
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
