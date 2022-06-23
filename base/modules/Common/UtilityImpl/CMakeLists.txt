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
#        MapSerialiser.h
#        MapSerialiser.cpp
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
        WaitForUtils.cpp)

target_include_directories(utilityimplobject PUBLIC
        ../../Common
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CURL_INCLUDE_DIR}
        ${BOOST_INCLUDE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nlohmann-json)

add_library(utilityimpl SHARED $<TARGET_OBJECTS:utilityimplobject>)

target_include_directories(utilityimpl PUBLIC
        ../../Common
        ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nlohmann-json
        )

SET_TARGET_PROPERTIES(utilityimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS utilityimpl
        LIBRARY DESTINATION files/base/lib64)

install(TARGETS utilityimpl LIBRARY DESTINATION ${THININSTALLER_DEPS_DIR_LIB64})