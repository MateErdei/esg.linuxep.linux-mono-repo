INCLUDE_DIRECTORIES(${CAPNPROTO_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

set( ENV{LD_LIBRARY_PATH} ${CAPNPROTO_LD_LIBRARY_PATH} )

execute_process(COMMAND bash -c "pwd")



add_custom_target(
        capnproto_definitions
        #COMMAND bash -c "LD_LIBRARY_PATH=/redist/binaries/linux11/input/capnproto/lib/ PATH=/redist/binaries/linux11/input/capnproto/bin/:$PATH ${CAPNPROTO_EXECUTABLE} compile -oc++ --src-prefix=${CMAKE_SOURCE_DIR}/modules/Common/EventTypesImpl/capnproto/ Directory.capnp"
        COMMAND bash -c "LD_LIBRARY_PATH=/redist/binaries/linux11/input/capnproto/lib/ PATH=/redist/binaries/linux11/input/capnproto/bin/:$PATH ${CAPNPROTO_EXECUTABLE} compile -oc++ --src-prefix=${CMAKE_CURRENT_LIST_DIR} ${CMAKE_CURRENT_LIST_DIR}/Directory.capnp"
)
        #COMMAND bash -c "LD_LIBRARY_PATH=/redist/binaries/linux11/input/capnproto/lib/ PATH=/redist/binaries/linux11/input/capnproto/bin/:$PATH ${CAPNPROTO_EXECUTABLE} compile -oc++ --src-prefix=../modules/Common/EventTypesImpl/capnproto/ ${file_to_compile}"


#function(compile_capnnp_files file_to_compile)
#
#    #execute_process(
#         # COMMAND bash -c "LD_LIBRARY_PATH=/redist/binaries/linux11/input/capnproto/lib/ PATH=/redist/binaries/linux11/input/capnproto/bin/:$PATH ${CAPNPROTO_EXECUTABLE} compile -oc++ --src-prefix=../modules/Common/EventTypesImpl/capnproto/ Directory.capnp"
#          #COMMAND bash -c "LD_LIBRARY_PATH=/redist/binaries/linux11/input/capnproto/lib/ PATH=/redist/binaries/linux11/input/capnproto/bin/:$PATH ${CAPNPROTO_EXECUTABLE} compile -oc++ --src-prefix=../modules/Common/EventTypesImpl/capnproto/ ${file_to_compile}"
#    #)
#endfunction(compile_capnnp_files)
#
#compile_capnnp_files("Common.capnp")
#compile_capnnp_files("Directory.capnp")
#compile_capnnp_files("Dns.capnp")
#compile_capnnp_files("Driver.capnp")
#compile_capnnp_files("Event.capnp")
#compile_capnnp_files("File.capnp")
#compile_capnnp_files("FileHash.capnp")
#compile_capnnp_files("FileProperty.capnp")
#compile_capnnp_files("Http.capnp")
#compile_capnnp_files("Image.capnp")
#compile_capnnp_files("Ip.capnp")
#compile_capnnp_files("Network.capnp")
#compile_capnnp_files("Process.capnp")
#compile_capnnp_files("ProcessProperty.capnp")
#compile_capnnp_files("Registry.capnp")
#compile_capnnp_files("System.capnp")
#compile_capnnp_files("Thread.capnp")
#compile_capnnp_files("Url.capnp")





