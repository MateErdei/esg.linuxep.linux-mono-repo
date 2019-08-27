add_library(utilityimplobject OBJECT
        MapUtils.cpp
        MapUtils.h
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
        VectorAsSet.h
        OrderedSet.h
        StrError.cpp
        StrError.h
        Logger.cpp
        Logger.h
        )

target_include_directories(utilityimplobject SYSTEM PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${BOOST_INCLUDE_DIR})

add_library(utilityimpl SHARED $<TARGET_OBJECTS:utilityimplobject> )
target_include_directories(utilityimpl PUBLIC ../../Common )

target_link_libraries(utilityimpl PUBLIC ${LOG4CPLUS_LIBRARY})

SET_TARGET_PROPERTIES( utilityimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS utilityimpl
        LIBRARY DESTINATION files/base/lib64)