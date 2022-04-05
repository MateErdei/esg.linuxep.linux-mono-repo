add_library(osutilitiesimpl SHARED
        ../OSUtilities/IIPUtils.h
        ../OSUtilities/ILocalIP.h
        ../OSUtilities/IDnsLookup.h
        ../OSUtilities/IIPUtils.cpp
        ../OSUtilities/IPlatformUtils.h
        ../OSUtilities/IPlatformUtilsException.h
        ../OSUtilities/ISystemUtils.h
        MACinfo.cpp
        MACinfo.h
        SXLMachineID.cpp
        SXLMachineID.h
        LocalIPImpl.cpp
        LocalIPImpl.h
        DnsLookupImpl.cpp
        DnsLookupImpl.h
        PlatformUtils.h
        PlatformUtils.cpp
        SystemUtils.h
        SystemUtils.cpp
        )

target_link_libraries(osutilitiesimpl PUBLIC sslimpl filesystemimpl utilityimpl applicationconfigurationimpl pthread)

include_directories(osutilitiesimpl BEFORE ../../../thirdparty/nlohmann-json ../CurlWrapper)

SET_TARGET_PROPERTIES(osutilitiesimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS osutilitiesimpl
        LIBRARY DESTINATION files/base/lib64)