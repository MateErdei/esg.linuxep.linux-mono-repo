
add_executable(sophos_watchdog main.cpp)
add_dependencies(sophos_watchdog watchdogimpl)
target_link_libraries(sophos_watchdog  watchdogimpl)

SET_TARGET_PROPERTIES( sophos_watchdog
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

install(TARGETS sophos_watchdog
        DESTINATION files/base/bin)
