cmake_minimum_required(VERSION 3.5)

file(GLOB_RECURSE python_SRC
        RELATIVE ${PYTHON_INPUT}/install64/lib/python3.7
        ${PYTHON_INPUT}/install64/lib/python3.7/*.py
        )

file(GLOB_RECURSE python_full_path_SRC
        ${PYTHON_INPUT}/install64/lib/python3.7/*.py
        )


file(GLOB_RECURSE mcs_router_DEPS
        ./*.py )

add_custom_target(mcsrouter
        DEPENDS ${CMAKE_BINARY_DIR}/mcsrouter.zip
        ${CMAKE_BINARY_DIR}/python37.zip
        ${CMAKE_BINARY_DIR}/lib-dynload
        ${CMAKE_BINARY_DIR}/pathtools
        ${CMAKE_BINARY_DIR}/watchdog
        ${CMAKE_BINARY_DIR}/Crypto
        ${CMAKE_BINARY_DIR}/certifi
        ${CMAKE_BINARY_DIR}/chardet
        ${CMAKE_BINARY_DIR}/idna
        ${CMAKE_BINARY_DIR}/requests
        ${CMAKE_BINARY_DIR}/six.py
        ${CMAKE_BINARY_DIR}/sseclient.py
        ${CMAKE_BINARY_DIR}/urllib3
        ${CMAKE_BINARY_DIR}/python3
        )
# this is to make sure there is a connection between the targets that clion knows it needs to build
# and these list of custom targets
add_dependencies(utilityimpl mcsrouter)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/mcsrouter.zip
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        # force compile all required mcs python code.
        # python3 changed the way to produce .pyc, and in order to allow for 'sourceless' distribution,
        # no python source can be present. https://www.python.org/dev/peps/pep-3147/#case-4-legacy-pyc-files-and-source-less-imports
        COMMAND ${PYTHON_INPUT}/install64/bin/python3 -m compileall -b -d ${CMAKE_BINARY_DIR}/mcsrouter  ${CMAKE_CURRENT_SOURCE_DIR}/mcsrouter
        COMMAND find mcsrouter | grep -v __pycache__ | grep "pyc" | zip ${CMAKE_BINARY_DIR}/mcsrouter.zip -@
        #COMMAND find mcsrouter | zip ${CMAKE_BINARY_DIR}/mcsrouter.zip -@
        DEPENDS ${mcs_router_DEPS}
        )

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/python37.zip
        WORKING_DIRECTORY ${PYTHON_INPUT}/install64/lib/python3.7
        COMMAND zip ${CMAKE_BINARY_DIR}/python37.zip ${python_SRC}
        DEPENDS ${python_full_path_SRC}
        )

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/lib-dynload
        WORKING_DIRECTORY ${PYTHON_INPUT}
        COMMAND cp -r ${PYTHON_INPUT}/install64/lib/python3.7/lib-dynload/ ${CMAKE_BINARY_DIR}
        )

function(add_python_library INPUT_PATH  PACKAGE_NAME)
    add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}
            WORKING_DIRECTORY ${INPUT_PATH}/..
            # Copy the whole directory as it may contain a mix of python and C files.
            COMMAND cp -r ${INPUT_PATH} ${CMAKE_BINARY_DIR}
            # Remove unwanted files.
            COMMAND find ${CMAKE_BINARY_DIR}/${PACKAGE_NAME} -name "*.pyc" -type f | xargs -r rm -f
            COMMAND find ${CMAKE_BINARY_DIR}/${PACKAGE_NAME} -name "*.pyo" -type f | xargs -r rm -f
            )

    if (IS_DIRECTORY ${CMAKE_BINARY_DIR}/${PACKAGE_NAME} )
        install(DIRECTORY ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}
                DESTINATION files/base/lib/python3.7/site-packages)
    else()
        install(FILES ${CMAKE_BINARY_DIR}/${PACKAGE_NAME}
                DESTINATION files/base/lib/python3.7/site-packages)
    endif()

endfunction(add_python_library)

add_python_library(${PYCRYPTODOME_INPUT} Crypto)

foreach (PYTHON_PACKAGE_NAME  watchdog pathtools certifi chardet idna requests six.py sseclient.py urllib3)
    add_python_library(${INPUT}/${PYTHON_PACKAGE_NAME} ${PYTHON_PACKAGE_NAME})
endforeach(PYTHON_PACKAGE_NAME)


add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/python3
        WORKING_DIRECTORY ${PYTHON_INPUT}
        COMMAND cp ${PYTHON_INPUT}/install64/bin/python3 ${CMAKE_BINARY_DIR}
        )

if ( PythonCoverage )
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mcsrouter
            DESTINATION files/base/lib64)
else()
    install(FILES ${CMAKE_BINARY_DIR}/mcsrouter.zip
            DESTINATION files/base/lib64)
endif()



install(FILES ${CMAKE_BINARY_DIR}/python37.zip
        DESTINATION files/base/lib64)

install(DIRECTORY ${CMAKE_BINARY_DIR}/lib-dynload
        DESTINATION files/base/lib/python3.7/)

install(FILES ${CMAKE_BINARY_DIR}/python3
        DESTINATION files/base/bin)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/engine/mcs_rootca.crt
        DESTINATION files/base/mcs/certs)
