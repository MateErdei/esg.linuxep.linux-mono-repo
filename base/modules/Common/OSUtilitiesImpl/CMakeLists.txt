add_library(osutilitiesimpl SHARED
        ../OSUtilities/IIPUtils.h
        ../OSUtilities/ILocalIP.h
        ../OSUtilities/IDnsLookup.h
        ../OSUtilities/IIPUtils.cpp
        MACinfo.cpp
        MACinfo.h
        SXLMachineID.cpp
        SXLMachineID.h
        LocalIPImpl.cpp
        LocalIPImpl.h
        DnsLookupImpl.cpp
        DnsLookupImpl.h
)

target_link_libraries(osutilitiesimpl PUBLIC sslimpl filesystemimpl utilityimpl applicationconfigurationimpl pthread)

SET_TARGET_PROPERTIES( osutilitiesimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS osutilitiesimpl
        LIBRARY DESTINATION files/base/lib64)