add_executable(sophos_diagnose main.cpp)
add_dependencies(sophos_diagnose diagnoseimpl)
target_link_libraries(sophos_diagnose diagnoseimpl filesystemimpl)

SET_TARGET_PROPERTIES(sophos_diagnose PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64")

install(TARGETS sophos_diagnose DESTINATION files/base/bin)
