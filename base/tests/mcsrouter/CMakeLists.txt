add_test (NAME TestAppProxyAdapter
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestAppProxyAdapter.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestAppProxyAdapter
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME TestComputer
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestComputer.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestComputer
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME TestMCS
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestMCS.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestMCS
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME TestMCSAdapter
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestMCSAdapter.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestMCSAdapter
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME TestMCSConnection
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestMCSConnection.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestMCSConnection
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME TestMCSResponse
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestMCSResponse.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestMCSResponse
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME TestPidFile
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestPidFile.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestPidFile
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME TestProxyAuthorization
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestProxyAuthorization.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestProxyAuthorization
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME TestSECObfuscation
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestSECObfuscation.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestSECObfuscation
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME TestStatusCache
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestStatusCache.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestStatusCache
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME TestStatusEvent
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestStatusEvent.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestStatusEvent
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME TestTimestamp
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestTimestamp.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestTimestamp
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")