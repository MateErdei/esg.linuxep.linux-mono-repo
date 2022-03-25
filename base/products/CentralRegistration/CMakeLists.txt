add_executable(centralregistration main.cpp)
target_include_directories(centralregistration PUBLIC ${PROJECT_SOURCE_DIR}/modules/CentralRegistration
        )

target_link_libraries(centralregistration PUBLIC centralregistrationimpl)

SET_TARGET_PROPERTIES( centralregistration
        PROPERTIES INSTALL_RPATH "$ORIGIN/../lib64"
        BUILD_RPATH "${SUL_LIBRARY_DIRS}"
        )

install(TARGETS centralregistration
        DESTINATION files/base/bin)