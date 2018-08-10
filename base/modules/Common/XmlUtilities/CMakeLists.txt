add_library(xmlutilities SHARED
        AttributesMap.cpp AttributesMap.h Logger.h)


target_include_directories(xmlutilities PUBLIC ${EXPAT_INCLUDE_DIR})
target_link_libraries(xmlutilities PUBLIC ${EXPAT_LIBRARY})

install(TARGETS xmlutilities
        LIBRARY DESTINATION files/base/lib64)