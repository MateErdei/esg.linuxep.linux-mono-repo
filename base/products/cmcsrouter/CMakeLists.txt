add_executable(central_registration main.cpp)

target_link_libraries(central_registration centralregistrationimpl)

SET_TARGET_PROPERTIES(central_registration PROPERTIES INSTALL_RPATH "$ORIGIN/../base/lib64")

install(TARGETS central_registration DESTINATION files/bin)

add_custom_target(
        mcstar ALL
        COMMAND mkdir -p mcslibrary/include mcslibrary/lib64
        COMMAND mkdir -p mcslibrary/include/cmcsrouter
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mcsimpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:HttpRequestsImpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:obfuscationimpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:utilityimpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:filesystemimpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:applicationconfigurationimpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:logging> mcslibrary/lib64
)

function(add_copy_directory base)
    file(GLOB_RECURSE tempfiles
            LIST_DIRECTORIES false
            ../../modules/${base}/*.h
            )
    add_custom_command(
            TARGET mcstar
            POST_BUILD
            #            COMMAND echo ${base}
            #            COMMAND echo ${tempfiles}
            COMMAND ${CMAKE_COMMAND} -E copy ${tempfiles} mcslibrary/include/${base}/
    )
endfunction()

add_copy_directory(cmcsrouter)

add_custom_command(
        TARGET mcstar
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar czf mcs.tar.gz mcslibrary
        COMMAND rm -rf mcslibrary
)