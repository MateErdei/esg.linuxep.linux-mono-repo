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


include_directories(suldownloaderimpl BEFORE ${SOPHLIB_INCLUDE_DIR} ${SUL_INPUT}/include ${SDDS3LIB_INPUT} ${GSL_INCLUDE} ${CURL_INCLUDE_DIR})


set(SDDS3_LIBRARY -L${SDDS3LIB_INPUT} -lsdds3 -ldelta -lpugixml -lutils -lminizip -lsdds3crypto -lminizip)
target_compile_options(suldownloaderimpl PUBLIC -Wno-unused-value -Wno-unused-parameter -Wno-maybe-uninitialized -Wno-stringop-overflow)
target_link_libraries(suldownloaderimpl PUBLIC
        ${SDDS3_LIBRARY}
        ${SOPHLIB_LIBRARIES}
        suldownloaderdata
        osutilitiesimpl
        HttpRequestsImpl
        curlwrapper
        stdc++fs
        -Wno-maybe-uninitialized -Wno-stringop-overflow)

SET_TARGET_PROPERTIES( suldownloaderimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs"
        )

install(TARGETS suldownloaderimpl
        LIBRARY DESTINATION ${BASE_DIST_PREFIX}/files/base/lib64)

install(TARGETS suldownloaderimpl LIBRARY DESTINATION ${THININSTALLER_DEPS_DIR_LIB64})

add_dependencies(suldownloaderimpl copy_libs)
