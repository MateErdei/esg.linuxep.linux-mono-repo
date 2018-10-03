add_library(osutilitiesimpl SHARED
        MACinfo.cpp MACinfo.h SXLMachineID.cpp SXLMachineID.h PidLockFile.cpp PidLockFile.h)

target_link_libraries(osutilitiesimpl PUBLIC sslimpl filesystemimpl applicationconfigurationimpl)

SET_TARGET_PROPERTIES( osutilitiesimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS osutilitiesimpl
        LIBRARY DESTINATION files/base/lib64)