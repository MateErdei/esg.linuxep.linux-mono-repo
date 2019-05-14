add_test (NAME Test_xml_helper
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Test_xml_helper.py
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
        )

set_tests_properties(Test_xml_helper
        PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/modules:$ENV{PYTHONPATH}")
