add_library(SulDownloader SHARED SULDownloader.cpp SulDownloader.h)
#Products.cpp Products.h ConfigurationData.cpp ConfigurationData.h ConnectionSetup.cpp ConnectionSetup.h ProductInformation.cpp ProductInformation.h
#/redist/binaries/linux10/input/SUL
set( REDIST /redist )
set( BINARIES_DIR ${REDIST}/binaries )
set( INPUT ${BINARIES_DIR}/linux10/input )
set( SUL_INPUT ${INPUT}/SUL )
set( BOOST_INPUT ${INPUT}/boost )
set( SUL_LIBRARY_DIR ${SUL_INPUT}/lib64 )
set( SUL_LIBRARY_DIRS ${SUL_LIBRARY_DIR}:${BOOST_INPUT}/lib64)

set (CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${SUL_LIBRARY_DIRS} )

set( SUL_LIBRARY -L${SUL_LIBRARY_DIR}  -lSUL)

target_link_libraries(SulDownloader PUBLIC ${SUL_LIBRARY})

include_directories(BEFORE ${SUL_INPUT}/include)