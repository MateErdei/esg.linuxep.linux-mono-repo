add_library(utilityimplobject OBJECT
        MessageUtility.cpp
        MessageUtility.h
        MessageUtility.h
        RegexUtilities.cpp
        RegexUtilities.h
        StringUtils.h
        iostr_utils.h
        TimeUtils.cpp
        TimeUtils.h
        UniformIntDistribution.cpp
        UniformIntDistribution.h
        Main.h
        VectorAsSet.cpp
        VectorAsSet.h)


target_include_directories(utilityimplobject SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR})
add_library(utilityimpl SHARED $<TARGET_OBJECTS:utilityimplobject> )
target_include_directories(utilityimpl SYSTEM BEFORE PUBLIC ${Protobuf_INCLUDE_DIR})
target_include_directories(utilityimpl PUBLIC ../../Common )
target_link_libraries(utilityimpl PUBLIC ${Protobuf_LIBRARIES})

SET_TARGET_PROPERTIES( utilityimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS utilityimpl
        LIBRARY DESTINATION files/base/lib64)