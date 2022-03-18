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
        suldownloaderdata/IWarehouseRepository.h
        SULDownloader.cpp
        SulDownloader.h
        ProductUninstaller.cpp
        ProductUninstaller.h
        Logger.cpp
        Logger.h
        )


include_directories(suldownloaderimpl BEFORE ${SUL_INPUT}/include)


set(SUL_LIBRARY -L${SUL_LIBRARY_DIR} -lSUL)
target_link_libraries(suldownloaderimpl PUBLIC suldownloaderdata ${SUL_LIBRARY} osutilitiesimpl)

SET_TARGET_PROPERTIES( suldownloaderimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS suldownloaderimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(suldownloaderimpl copy_libs)
