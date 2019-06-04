add_test (NAME TestXmlHelper
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/TestXmlHelper.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(TestXmlHelper
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/modules:$ENV{PYTHONPATH}")
