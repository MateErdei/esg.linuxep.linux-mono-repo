cmake_minimum_required(VERSION 3.8)

add_subdirectory(suldownloaderdata)

add_library(suldownloaderimplobject OBJECT
        sdds3/SDDS3Repository.cpp
        sdds3/SDDS3Repository.h
        sdds3/Sdds3RepositoryFactory.cpp
        sdds3/Sdds3RepositoryFactory.h
        sdds3/SDDS3Utils.cpp
        sdds3/SDDS3Utils.h
        sdds3/ISdds3Wrapper.h
        sdds3/ISignatureVerifierWrapper.h
        sdds3/ISusRequester.h
        sdds3/SDDS3CacheUtils.cpp
        sdds3/SDDS3CacheUtils.h
        sdds3/Sdds3Wrapper.h
        sdds3/Sdds3Wrapper.cpp
        sdds3/SignatureVerifierWrapper.cpp
        sdds3/SignatureVerifierWrapper.h
        sdds3/SusRequestParameters.h
        sdds3/SusRequester.h
        sdds3/SusRequester.cpp
        sdds3/Jwt.h
        sdds3/Jwt.cpp
        sdds3/JwtVerificationException.h
        sdds3/Logger.h
        sdds3/Logger.cpp
        common/Logger.h
        common/Logger.cpp
        SULDownloader.cpp
        SulDownloader.h
        ProductUninstaller.cpp
        ProductUninstaller.h
        common/SulDownloaderUtils.cpp
        common/SulDownloaderUtils.h
        ../Common/UpdateUtilities/InstalledFeatures.h
        ../Common/UpdateUtilities/InstalledFeatures.cpp
        )

target_include_directories(suldownloaderimplobject
        PUBLIC
        ${PROJECT_SOURCE_DIR}/modules
        ${NLOHMANN_JSON_INCLUDE_DIR}
        ${GSL_INCLUDE}
        ${LOG4CPLUS_INCLUDE_DIR}
        ${CURL_INPUT}/include/curl
        ${SOPHLIB_INCLUDE}
        ${AUTOVERSION_INCLUDE}
        )

add_library(suldownloaderimpl_static STATIC
        $<TARGET_OBJECTS:suldownloaderimplobject>
        $<TARGET_OBJECTS:suldownloaderdataobject>
        $<TARGET_OBJECTS:HttpRequestsImplobject>
        $<TARGET_OBJECTS:filesystemimplobject>
        $<TARGET_OBJECTS:processimplobject>
        $<TARGET_OBJECTS:threadsobject>
        $<TARGET_OBJECTS:utilityimplobject>
        $<TARGET_OBJECTS:applicationconfigurationimplobject>
        $<TARGET_OBJECTS:loggingobject>
        $<TARGET_OBJECTS:obfuscationimplobject>
        $<TARGET_OBJECTS:protobufutilobject>
        $<TARGET_OBJECTS:policyobject>
        $<TARGET_OBJECTS:ProxyUtilsLibobject>
        $<TARGET_OBJECTS:sslimplobject>
        $<TARGET_OBJECTS:curlwrapperobject>
        $<TARGET_OBJECTS:downloadreportobject>
        $<TARGET_OBJECTS:updateutilitiesobject>
        )
target_link_libraries(suldownloaderimpl_static PRIVATE
        ${LOG4CPLUS_LIBRARY}
        ${Protobuf_LIBRARIES}
        openssl_crypto_static
        curl_library_static
        cap
        sophlib_sdds3
        )

add_library(suldownloaderimpl SHARED $<TARGET_OBJECTS:suldownloaderimplobject>)
target_link_libraries(suldownloaderimpl PUBLIC
        logging
        HttpRequestsImpl
        curlwrapper
        sophlib_sdds3
        downloadreport
        updateutilities
        )

SET_TARGET_PROPERTIES( suldownloaderimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

add_dependencies(suldownloaderimpl copy_libs)
