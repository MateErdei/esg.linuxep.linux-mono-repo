
add_executable(sophos_watchdog main.cpp)
add_dependencies(sophos_watchdog watchdogimpl)
target_link_libraries(sophos_watchdog  watchdogimpl)


install(TARGETS sophos_watchdog
        DESTINATION files/base/bin)
