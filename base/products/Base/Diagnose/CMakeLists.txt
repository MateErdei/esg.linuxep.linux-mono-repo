add_executable(sophos_diagnose main.cpp)

target_link_libraries(sophos_diagnose diagnoseimpl ${ZeroMQ_LIBRARY})

SET_TARGET_PROPERTIES(sophos_diagnose PROPERTIES INSTALL_RPATH "$ORIGIN/../base/lib64")

install(TARGETS sophos_diagnose DESTINATION ${BASE_DIST_PREFIX}/files/bin)
