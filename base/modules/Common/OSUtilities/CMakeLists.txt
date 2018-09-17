add_library(osutilities SHARED
        MACinfo.cpp MACinfo.h SXLMachineID.cpp SXLMachineID.h)

target_link_libraries(osutilities PUBLIC sslimpl filesystemimpl applicationconfigurationimpl)

SET_TARGET_PROPERTIES( osutilities
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS osutilities
        LIBRARY DESTINATION files/base/lib64)