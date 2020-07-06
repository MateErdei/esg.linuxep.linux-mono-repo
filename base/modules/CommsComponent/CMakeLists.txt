add_library(commscomponentimpl SHARED Comms.cpp Logger.cpp Logger.h Comms.h)

target_link_libraries(commscomponentimpl PUBLIC logging processimpl)

SET_TARGET_PROPERTIES( commscomponentimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS commscomponentimpl
        LIBRARY DESTINATION files/base/lib64)

