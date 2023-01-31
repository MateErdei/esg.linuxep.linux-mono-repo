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
        CloudMetadataConverters.h)

target_link_libraries(osutilitiesimpl PUBLIC sslimpl filesystemimpl utilityimpl applicationconfigurationimpl pthread)

target_include_directories(osutilitiesimpl PUBLIC BEFORE ${NLOHMANN_JSON_INCLUDE_DIR} ../CurlWrapper)

SET_TARGET_PROPERTIES(osutilitiesimpl
        PROPERTIES INSTALL_RPATH "$ORIGIN")

install(TARGETS osutilitiesimpl
        LIBRARY DESTINATION base/files/base/lib64)

install(TARGETS osutilitiesimpl LIBRARY DESTINATION ${THININSTALLER_DEPS_DIR_LIB64})
