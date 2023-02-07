add_library(ziputiltiesobject OBJECT
        ZipUtils.cpp
        ZipUtils.h)

include_directories(ziputiltiesobject PUBLIC ${LOG4CPLUS_INCLUDE_DIR})
add_library(ziputilities SHARED $<TARGET_OBJECTS:ziputiltiesobject>)
target_link_libraries(ziputilities PUBLIC
        ${LOG4CPLUS_LIBRARY})

install(TARGETS ziputilities
        LIBRARY DESTINATION files/base/lib64)