
add_subdirectory(suldownloaderdata)

add_library(suldownloaderimpl SHARED
    SULRaii.h
    SULUtils.cpp
    SULUtils.h
    WarehouseRepository.cpp
    WarehouseRepository.h
    WarehouseRepositoryFactory.cpp
    WarehouseRepositoryFactory.h
    suldownloaderdata/IWarehouseRepository.h
    SULDownloader.cpp
    SulDownloader.h
    ProductUninstaller.cpp
    ProductUninstaller.h)


set(SUL_LIBRARY -L${SUL_LIBRARY_DIR} -lSUL)
target_link_libraries(suldownloaderimpl PUBLIC suldownloaderdata ${SUL_LIBRARY} )


include_directories(suldownloaderimpl BEFORE ${SUL_INPUT}/include)

SET_TARGET_PROPERTIES( suldownloaderimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )


install(TARGETS suldownloaderimpl
        LIBRARY DESTINATION files/base/lib64)
