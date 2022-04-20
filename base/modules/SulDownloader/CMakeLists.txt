cmake_minimum_required(VERSION 3.8)

add_subdirectory(suldownloaderdata)

add_library(suldownloaderimpl SHARED
        warehouse/SULRaii.h
        warehouse/SULUtils.cpp
        warehouse/SULUtils.h
        warehouse/WarehouseRepository.cpp
        warehouse/WarehouseRepository.h
        warehouse/WarehouseRepositoryFactory.cpp
        warehouse/WarehouseRepositoryFactory.h
        sdds3/SDDS3Repository.cpp
        sdds3/SDDS3Repository.h
        sdds3/Sdds3RepositoryFactory.cpp
        sdds3/Sdds3RepositoryFactory.h
        sdds3/SDDS3Utils.cpp
        sdds3/SDDS3Utils.h
        sdds3/ISdds3Wrapper.h
        sdds3/Sdds3Wrapper.h
        sdds3/Sdds3Wrapper.cpp
        suldownloaderdata/IWarehouseRepository.h
        SULDownloader.cpp
        SulDownloader.h
        ProductUninstaller.cpp
        ProductUninstaller.h
        Logger.cpp
        Logger.h
        )


include_directories(suldownloaderimpl BEFORE ${SOPHLIB_INCLUDE_DIR} ${SUL_INPUT}/include ${SDDS3LIB_INPUT} ${GSL_INCLUDE} ${CURL_INCLUDE_DIR})


set(SUL_LIBRARY -L${SUL_LIBRARY_DIR} -lSUL)
set(SDDS3_LIBRARY -L${SDDS3LIB_INPUT} -lsdds3 -ldelta -lpugixml -lutils -lminizip -lsdds3crypto -lminizip)
target_compile_options(suldownloaderimpl PUBLIC -Wno-unused-value -Wno-unused-parameter -Wno-maybe-uninitialized -Wno-stringop-overflow)
target_link_libraries(suldownloaderimpl PUBLIC stdc++fs suldownloaderdata ${SUL_LIBRARY} ${SDDS3_LIBRARY} ${SOPHLIB_LIBRARIES} osutilitiesimpl -Wno-maybe-uninitialized -Wno-stringop-overflow)

SET_TARGET_PROPERTIES( suldownloaderimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS suldownloaderimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(suldownloaderimpl copy_libs)
