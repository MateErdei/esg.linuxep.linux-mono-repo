cmake_minimum_required(VERSION 3.8)

add_subdirectory(suldownloaderdata)

add_library(suldownloaderimpl SHARED
        sdds3/SDDS3Repository.cpp
        sdds3/SDDS3Repository.h
        sdds3/Sdds3RepositoryFactory.cpp
        sdds3/Sdds3RepositoryFactory.h
        sdds3/SDDS3Utils.cpp
        sdds3/SDDS3Utils.h
        sdds3/ISdds3Wrapper.h
        sdds3/Sdds3Wrapper.h
        sdds3/Sdds3Wrapper.cpp
        sdds3/SusRequestParameters.h
        sdds3/SusRequester.h
        sdds3/SusRequester.cpp
        sdds3/Logger.h
        sdds3/Logger.cpp
        SULDownloader.cpp
        SulDownloader.h
        ProductUninstaller.cpp
        ProductUninstaller.h
        ../Common/UpdateUtilities/InstalledFeatures.h
        ../Common/UpdateUtilities/InstalledFeatures.cpp
        Logger.cpp
        Logger.h
        )


include_directories(suldownloaderimpl BEFORE ${GSL_INCLUDE})

target_link_libraries(suldownloaderimpl PUBLIC
        logging
        sophlib_sdds3
        suldownloaderdata
        HttpRequestsImpl
        curlwrapper
        )

SET_TARGET_PROPERTIES( suldownloaderimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS suldownloaderimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

install(TARGETS suldownloaderimpl LIBRARY DESTINATION ${THININSTALLER_DEPS_DIR_LIB64})

add_dependencies(suldownloaderimpl copy_libs)
