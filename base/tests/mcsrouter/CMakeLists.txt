add_test (NAME PythonTestAppProxyAdapter
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestAppProxyAdapter.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestAppProxyAdapter
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME PythonTestComputer
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestComputer.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestComputer
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME PythonTestMCS
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestMCS.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestMCS
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME PythonTestMCSAdapter
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestMCSAdapter.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestMCSAdapter
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME PythonTestMCSConnection
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestMCSConnection.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestMCSConnection
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME PythonTestMCSResponse
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestMCSResponse.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestMCSResponse
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME PythonTestPidFile
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestPidFile.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestPidFile
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME PythonTestProxyAuthorization
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestProxyAuthorization.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestProxyAuthorization
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME PythonTestSECObfuscation
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestSECObfuscation.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestSECObfuscation
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME PythonTestStatusCache
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestStatusCache.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestStatusCache
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME PythonTestStatusEvent
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestStatusEvent.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestStatusEvent
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

add_test (NAME PythonTestTimestamp
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestTimestamp.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestTimestamp
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")


add_test (NAME PythonTestUtils
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestUtils.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestUtils
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")


add_test (NAME PythonTestGenericAdapter
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestGenericAdapter.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(PythonTestGenericAdapter
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")


