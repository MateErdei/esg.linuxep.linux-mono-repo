add_library(ziputiltiesobject OBJECT
        ZipUtils.cpp
        ZipUtils.h
        $<TARGET_OBJECTS:filesystemimplobject>)


target_include_directories(ziputiltiesobject PUBLIC ${LOG4CPLUS_INCLUDE_DIR} ${MINIZIP_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR})
add_library(ziputilities SHARED $<TARGET_OBJECTS:ziputiltiesobject>)
target_link_libraries(ziputilities PUBLIC
        filesystemimpl ${LOG4CPLUS_LIBRARY} ${ZLIB_LIBRARY} ${MINIZIP_LIBRARY})

install(TARGETS ziputilities
        LIBRARY DESTINATION files/base/lib64)
