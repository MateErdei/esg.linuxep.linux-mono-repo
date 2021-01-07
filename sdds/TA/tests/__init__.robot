*** Settings ***
Documentation    Setup for tests

Suite Setup   Global Setup Tasks

Library    ../libs/PathManager.py

Library           OperatingSystem

*** Keywords ***
Global Setup Tasks
    ${placeholder} =  PathManager.get_support_file_path
    Set Global Variable  ${SUPPORT_FILES}     ${placeholder}

    ${placeholder} =  Get Environment Variable  SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}  ${placeholder}

    Set Global Variable  ${BASE_LOGS_DIR}               ${SOPHOS_INSTALL}/logs/base
