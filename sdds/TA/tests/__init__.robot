*** Settings ***
Documentation    Setup for tests

Suite Setup   Global Setup Tasks

Library    ../libs/PathManager.py

*** Keywords ***
Global Setup Tasks
    ${placeholder} =  PathManager.get_support_file_path
    Set Global Variable  ${SUPPORT_FILES}     ${placeholder}
