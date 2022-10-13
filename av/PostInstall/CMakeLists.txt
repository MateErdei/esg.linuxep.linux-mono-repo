# Other post-install steps can be added below

# tap_test_output can be used to store items that are needed in TAP tests, e.g. binaries that are created as
# part of the build but should be run in TAP rather that as unit tests.
INSTALL(CODE "MESSAGE(STATUS \"Archiving tap_test_output directory\")
    execute_process(
      COMMAND tar -zcvf ${OUTPUT}/tap_test_output.tar.gz -C ${CMAKE_BINARY_DIR} tap_test_output/
        RESULT_VARIABLE STATUS)
    if(STATUS AND NOT STATUS EQUAL 0)
        message(FATAL_ERROR \"Failed archive tap_test_output directory: ${STATUS}\")
    endif()")
