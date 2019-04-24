add_executable(sophos_diagnose main.cpp)

target_link_libraries(sophos_diagnose diagnoseimpl)

SET_TARGET_PROPERTIES(sophos_diagnose PROPERTIES INSTALL_RPATH "$ORIGIN/../base/lib64")

install(TARGETS sophos_diagnose DESTINATION files/bin)
