add_library(xmlutilities SHARED
        AttributesMap.cpp AttributesMap.h Logger.h Logger.cpp)


target_include_directories(xmlutilities PUBLIC ${EXPAT_INCLUDE_DIR} ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(xmlutilities PUBLIC
        ${EXPAT_LIBRARY}
        ${LOG4CPLUS_LIBRARY})

install(TARGETS xmlutilities
        LIBRARY DESTINATION files/base/lib64)