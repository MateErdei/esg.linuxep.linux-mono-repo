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
        StringUtils.cpp
        ConfigException.h
        ConfigException.cpp
        FileUtils.cpp
        FileUtils.h
        ${CMAKE_CURRENT_BINARY_DIR}/ProjectNames.cpp
        SophosSplUsers.h
        SophosSplUsers.cpp
        ISophosSplUsers.h
        WaitForUtils.h
        WaitForUtils.cpp
        SystemExecutableUtils.cpp
        SystemExecutableUtils.h
        Uuid.cpp
        Uuid.h
        )
target_include_directories(utilityimplobject PUBLIC ../../Common ${CMAKE_SOURCE_DIR}/modules ${CMAKE_CURRENT_SOURCE_DIR} ${BOOST_INCLUDE_DIR})
add_library(utilityimpl SHARED)
target_link_libraries(utilityimpl PUBLIC utilityimplobject)

SET_TARGET_PROPERTIES(utilityimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS utilityimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
