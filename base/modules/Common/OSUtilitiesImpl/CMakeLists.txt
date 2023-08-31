add_library(osutilitiesimpl SHARED
        ../OSUtilities/IIPUtils.h
        ../OSUtilities/ILocalIP.h
        ../OSUtilities/IDnsLookup.h
        IIPUtils.cpp
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
        CloudMetadataConverters.h
        CloudMetadataConverters.cpp)

target_link_libraries(osutilitiesimpl PUBLIC filesystemimpl utilityimpl applicationconfigurationimpl pthread)

target_include_directories(osutilitiesimpl PUBLIC BEFORE ${NLOHMANN_JSON_INCLUDE_DIR} ../CurlWrapper)

SET_TARGET_PROPERTIES(osutilitiesimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS osutilitiesimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)
