add_library(managementagentloggerimpl SHARED
        Logger.h
        Logger.cpp)
target_include_directories(managementagentloggerimpl PUBLIC  ../../Common ${LOG4CPLUS_INCLUDE_DIR})
target_link_libraries(managementagentloggerimpl PUBLIC
        ${LOG4CPLUS_LIBRARY}
        logging
        )

SET_TARGET_PROPERTIES( managementagentloggerimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS managementagentloggerimpl
        LIBRARY DESTINATION files/base/lib64)
