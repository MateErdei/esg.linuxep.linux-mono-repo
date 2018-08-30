
add_subdirectory(suldownloaderdata)

add_library(suldownloaderimpl SHARED
    Logger.cpp
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


include_directories(suldownloaderimpl BEFORE ${SUL_INPUT}/include ${LOG4CPLUS_INCLUDE_DIR})


set(SUL_LIBRARY -L${SUL_LIBRARY_DIR} -lSUL)
target_link_libraries(suldownloaderimpl PUBLIC suldownloaderdata logging ${SUL_LIBRARY} )

SET_TARGET_PROPERTIES( suldownloaderimpl
        PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "${CMAKE_BINARY_DIR}/libs:${SUL_LIBRARY_DIRS}"
        )

install(TARGETS suldownloaderimpl
        LIBRARY DESTINATION files/base/lib64)

add_dependencies(suldownloaderimpl copy_libs)
