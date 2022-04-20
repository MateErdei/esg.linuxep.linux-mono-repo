add_executable(central_registration main.cpp)

target_link_libraries(central_registration centralregistrationimpl)

SET_TARGET_PROPERTIES(central_registration PROPERTIES INSTALL_RPATH "$ORIGIN/../base/lib64")

install(TARGETS central_registration DESTINATION files/bin)
