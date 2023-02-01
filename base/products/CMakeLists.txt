execute_process(
        COMMAND date +%Y-%m-%d
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE BUILD_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE)

add_subdirectory(Base)
add_subdirectory(distribution)
add_subdirectory(PluginApi)
add_subdirectory(ResponseActions)
add_subdirectory(SystemProductTestOutput)
add_subdirectory(ThininstallerDependencies)



