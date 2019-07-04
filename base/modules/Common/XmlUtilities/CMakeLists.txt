add_library(xmlutilitiesobject OBJECT
        AttributesMap.cpp AttributesMap.h Logger.h Logger.cpp)


include_directories(xmlutilitiesobject PUBLIC ${EXPAT_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR})
add_library(xmlutilities SHARED $<TARGET_OBJECTS:xmlutilitiesobject>)
target_link_libraries(xmlutilities PUBLIC
        ${EXPAT_LIBRARY}
        ${LOG4CPLUS_LIBRARY})

install(TARGETS xmlutilities
        LIBRARY DESTINATION files/base/lib64)
