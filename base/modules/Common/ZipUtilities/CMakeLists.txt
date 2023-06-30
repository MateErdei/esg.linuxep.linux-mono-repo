add_library(ziputiltiesobject OBJECT
        IZipUtils.h
        Logger.cpp
        Logger.h
        UnzipFileWrapper.cpp
        UnzipFileWrapper.h
        ZipUtils.cpp
        ZipUtils.h
        $<TARGET_OBJECTS:filesystemimplobject>)

target_include_directories(ziputiltiesobject PUBLIC ${CMAKE_SOURCE_DIR} ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(ziputiltiesobject PUBLIC minizip)
add_library(ziputilities SHARED $<TARGET_OBJECTS:ziputiltiesobject>)
target_link_libraries(ziputilities PUBLIC
        ${LOG4CPLUS_LIBRARY} minizip)

SET_TARGET_PROPERTIES(ziputilities
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS ziputilities
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
