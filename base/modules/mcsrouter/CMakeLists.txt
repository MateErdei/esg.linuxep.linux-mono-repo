cmake_minimum_required(VERSION 3.5)

file(GLOB_RECURSE mcs_router_SRC
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ./*.pyc )

file(GLOB_RECURSE python_SRC
        RELATIVE ${PYTHON_INPUT}/install64/lib/python2.7
        ${PYTHON_INPUT}/install64/lib/python2.7/*.py )


file(GLOB_RECURSE mcs_router_DEPS
        ./*.py )

add_custom_target(mcsrouter ALL
        DEPENDS mcsrouter.zip
        python27.zip
        pythonLibDynload
        pathtools
        watchdog
        pycryptodome
        python
        )

add_custom_command(OUTPUT mcsrouter.zip
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        # force compile all required mcs python code.
        COMMAND python -m compileall ${CMAKE_CURRENT_SOURCE_DIR}/mcsrouter
        COMMAND zip ${CMAKE_BINARY_DIR}/mcsrouter.zip ${mcs_router_SRC}
        # clean up compiled files
        COMMAND find . -name "*.pyc" | xargs rm -f
        DEPENDS ${mcs_router_DEPS}
        )

add_custom_command(OUTPUT python27.zip
        WORKING_DIRECTORY ${INPUT}/python/install64/lib/python2.7/
        COMMAND zip ${CMAKE_BINARY_DIR}/python27.zip ${python_SRC}
        )

add_custom_command(OUTPUT pythonLibDynload
        WORKING_DIRECTORY ${INPUT}
        COMMAND cp -r ${INPUT}/python/install64/lib/python2.7/lib-dynload/ ${CMAKE_BINARY_DIR}
        )

add_custom_command(OUTPUT pathtools
        WORKING_DIRECTORY ${INPUT}
        # Copy the whole directory as it may contain a mix of python and C files.
        COMMAND cp -r ${INPUT}/pathtools ${CMAKE_BINARY_DIR}
        # Remove unwanted files.
        COMMAND find ${CMAKE_BINARY_DIR}/pathtools -name "*.pyc" -type f | xargs -r rm -f
        COMMAND find ${CMAKE_BINARY_DIR}/pathtools -name "*.pyo" -type f | xargs -r rm -f
        )

add_custom_command(OUTPUT watchdog
        WORKING_DIRECTORY ${INPUT}
        # Copy the whole directory as it may contain a mix of python and C files.
        COMMAND cp -r ${INPUT}/watchdog ${CMAKE_BINARY_DIR}
        # Remove unwanted files.
        COMMAND find ${CMAKE_BINARY_DIR}/watchdog -name "*.pyc" -type f | xargs -r rm -f
        COMMAND find ${CMAKE_BINARY_DIR}/watchdog -name "*.pyo" -type f | xargs -r rm -f
        )

add_custom_command(OUTPUT pycryptodome
        WORKING_DIRECTORY ${INPUT}
        # Copy the whole directory as it may contain a mix of python and C files.
        COMMAND cp -r ${PYCRYPTODOME_INPUT}/Crypto ${CMAKE_BINARY_DIR}
        # Remove unwanted files.
        COMMAND find ${PYCRYPTODOME_INPUT}/Crypto -name "*.pyc" -type f | xargs -r rm -f
        COMMAND find ${PYCRYPTODOME_INPUT}/Crypto -name "*.pyo" -type f | xargs -r rm -f
        )

add_custom_command(OUTPUT python
        WORKING_DIRECTORY ${INPUT}
        COMMAND cp ${PYTHON_INPUT}/install64/bin/python ${CMAKE_BINARY_DIR}
        )

install(FILES ${CMAKE_BINARY_DIR}/mcsrouter.zip
        DESTINATION files/base/lib64)

install(FILES ${CMAKE_BINARY_DIR}/python27.zip
        DESTINATION files/base/lib64)

install(DIRECTORY ${CMAKE_BINARY_DIR}/lib-dynload
        DESTINATION files/base/lib/python2.7/)

install(DIRECTORY ${CMAKE_BINARY_DIR}/pathtools
        DESTINATION files/base/lib/python2.7/site-packages)

install(DIRECTORY ${CMAKE_BINARY_DIR}/watchdog
        DESTINATION files/base/lib/python2.7/site-packages)

install(DIRECTORY ${CMAKE_BINARY_DIR}/Crypto
        DESTINATION files/base/lib/python2.7/site-packages)

install(FILES ${CMAKE_BINARY_DIR}/python
        DESTINATION files/base/bin)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/engine/mcs_rootca.crt
        DESTINATION files/base/mcs/certs)
