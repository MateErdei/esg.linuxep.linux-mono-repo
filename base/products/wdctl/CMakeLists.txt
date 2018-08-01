
add_executable(wdctl main.cpp)
add_dependencies(wdctl wdctlimpl)
target_link_libraries(wdctl wdctlimpl)

SET_TARGET_PROPERTIES( wdctl
        PROPERTIES INSTALL_RPATH "$ORIGIN/../base/lib64")

install(TARGETS wdctl
        DESTINATION files/bin)
