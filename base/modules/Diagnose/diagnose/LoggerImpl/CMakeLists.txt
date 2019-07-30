add_library(diagnoseloggerimpl SHARED
        Logger.h
        Logger.cpp)
target_include_directories(diagnoseloggerimpl PUBLIC  ../../Common ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(diagnoseloggerimpl PUBLIC
        ${LOG4CPLUS_LIBRARY}
        logging
        )

SET_TARGET_PROPERTIES( diagnoseloggerimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS diagnoseloggerimpl
        LIBRARY DESTINATION files/base/lib64)
