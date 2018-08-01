
add_executable(wdctl main.cpp)
target_link_libraries(wdctl)

SET_TARGET_PROPERTIES( wdctl
        PROPERTIES INSTALL_RPATH "$ORIGIN/../base/lib64")

install(TARGETS wdctl
        DESTINATION files/bin)
