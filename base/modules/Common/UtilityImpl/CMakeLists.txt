add_library(utilityimpl SHARED
        MessageUtility.cpp
        MessageUtility.h
        RegexUtilities.cpp
        RegexUtilities.h
        StringUtils.h
        iostr_utils.h)

target_include_directories(utilityimpl PUBLIC ../../Common ${Protobuf_INCLUDE_DIR})
target_link_libraries(utilityimpl PUBLIC ${Protobuf_LIBRARIES})

SET_TARGET_PROPERTIES( utilityimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS utilityimpl
        LIBRARY DESTINATION files/base/lib64)