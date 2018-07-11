add_library(utilityimpl SHARED
        MessageUtility.cpp
        MessageUtility.h)

target_include_directories(utilityimpl PUBLIC ../../Common ${Protobuf_INCLUDE_DIR})
target_link_libraries(utilityimpl PUBLIC ${Protobuf_LIBRARIES})