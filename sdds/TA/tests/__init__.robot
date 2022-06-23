*** Settings ***
Documentation    Setup for tests

Suite Setup   Global Setup Tasks

Library    ../libs/PathManager.py

Library           OperatingSystem

*** Keywords ***
Global Setup Tasks
    ${placeholder} =  PathManager.get_support_file_path
    Set Global Variable  ${SUPPORT_FILES}     ${placeholder}
    ${placeholder} =  PathManager.get_libs_file_path
    Set Global Variable  ${LIB_FILES}     ${placeholder}
    ${placeholder} =  Get Environment Variable  SOPHOS_INSTALL  default=/opt/sophos-spl
    Set Global Variable  ${SOPHOS_INSTALL}  ${placeholder}

    Set Global Variable  ${BASE_LOGS_DIR}               ${SOPHOS_INSTALL}/logs/base
    Set Global Variable  ${PLUGINS_DIR}                 ${SOPHOS_INSTALL}/plugins
    Set Global Variable  ${MTR_DIR}                     ${PLUGINS_DIR}/mtr
    Set Global Variable  ${EDR_DIR}                     ${PLUGINS_DIR}/edr
    Set Global Variable  ${SSPLAV_DIR}                  ${PLUGINS_DIR}/av
