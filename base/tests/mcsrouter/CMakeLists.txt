file(GLOB_RECURSE python_tests
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/Test*.py
        )
foreach (python_test  ${python_tests})
    string(REPLACE ".py" "" output ${python_test})
    message("All python test files ${output}" )
    add_test (NAME Python${output}
            COMMAND ${PYTHON_COMMAND} ${CMAKE_CURRENT_SOURCE_DIR}/${python_test}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/modules
            )

    set_tests_properties(Python${output}
            PROPERTIES ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/build64:${PROJECT_SOURCE_DIR}/modules")

endforeach(python_test)



