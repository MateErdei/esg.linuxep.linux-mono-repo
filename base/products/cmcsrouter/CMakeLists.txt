add_custom_target(
        mcstar ALL
        COMMAND mkdir -p mcslibrary/include mcslibrary/lib64
        COMMAND mkdir -p mcslibrary/include/cmcsrouter
        COMMAND mkdir -p mcslibrary/include/CentralRegistration
        COMMAND mkdir -p mcslibrary/include/Common/OSUtilities
        COMMAND mkdir -p mcslibrary/include/Common/CurlWrapper
        COMMAND mkdir -p mcslibrary/include/Common/HttpRequestsImpl
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mcsimpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:HttpRequestsImpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:obfuscationimpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:utilityimpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:filesystemimpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:applicationconfigurationimpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:logging> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:centralregistrationimpl> mcslibrary/lib64
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:HttpRequestsImpl> mcslibrary/lib64
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
add_copy_directory(CentralRegistration)
add_copy_directory(Common/OSUtilities)
add_copy_directory(Common/CurlWrapper)
add_copy_directory(Common/HttpRequestsImpl)

add_custom_command(
        TARGET mcstar
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E tar czf mcs.tar.gz mcslibrary
        COMMAND rm -rf mcslibrary
)