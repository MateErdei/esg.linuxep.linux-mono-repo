add_library(ziputiltiesobject OBJECT
        Logger.cpp
        Logger.h
        UnzipFileWrapper.cpp
        UnzipFileWrapper.h
        ZipUtils.cpp
        ZipUtils.h
        $<TARGET_OBJECTS:filesystemimplobject>)


target_include_directories(ziputiltiesobject PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${MINIZIP_INCLUDE_DIR})
add_library(ziputilities SHARED $<TARGET_OBJECTS:ziputiltiesobject>)
target_link_libraries(ziputilities PUBLIC
        ${LOG4CPLUS_LIBRARY} ${ZLIB_LIBRARY} ${MINIZIP_LIBRARY})

SET_TARGET_PROPERTIES(ziputilities
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS ziputilities
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
