
#message("directory : " ${PROJECT_BINARY_DIR}/modules/Watchdog)
add_executable(Watchdog main.cpp)
add_dependencies(Watchdog watchdogimpl)
target_include_directories(Watchdog  PUBLIC ${PROJECT_SOURCE_DIR}/modules/Watchdog)

set (CMAKE_EXE_LINKER_FLAGS -Wl,-rpath,${SUL_LIBRARY_DIRS} )

target_link_libraries(Watchdog  watchdogimpl)

install(TARGETS Watchdog
        DESTINATION files/base/bin)
