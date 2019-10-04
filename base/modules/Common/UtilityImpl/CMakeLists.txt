configure_file(ProjectNames.cpp.in ProjectNames.cpp @ONLY)
add_library(utilityimplobject OBJECT
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
        Factory.h
        ProjectNames.h
        ${CMAKE_CURRENT_BINARY_DIR}/ProjectNames.cpp)
target_include_directories(utilityimplobject PUBLIC ../../Common ${CMAKE_CURRENT_SOURCE_DIR})
add_library(utilityimpl SHARED $<TARGET_OBJECTS:utilityimplobject> )
target_include_directories(utilityimpl PUBLIC ../../Common ${CMAKE_CURRENT_BINARY_DIR})

SET_TARGET_PROPERTIES( utilityimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS utilityimpl
        LIBRARY DESTINATION files/base/lib64)