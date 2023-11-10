*** Settings ***
Library     ${COMMON_TEST_LIBS}/FullInstallerUtils.py

Resource    DeviceIsolationResources.robot

Suite Setup     Device Isolation Suite Setup
Suite Teardown  Device Isolation Suite Teardown

Test Setup     Device Isolation Test Setup
Test Teardown  Device Isolation Test Teardown

*** Test Cases ***
Device Isolation Log Files Are Saved When Downgrading
	Register Cleanup  Install Device Isolation Directly from SDDS
    Downgrade Device Isolation

    # check that the log folder only contains the downgrade-backup directory
    ${log_dir} =  List Directory  ${SOPHOS_INSTALL}/plugins/deviceisolation/log
    Length Should Be  ${log_dir}  ${1}  log directory contains more than downgrade-backup
    Should Contain  ${log_dir}  downgrade-backup
    ${log_dir_files} =  List Files In Directory  ${SOPHOS_INSTALL}/plugins/deviceisolation/log
    Log  ${log_dir_files}
    Length Should Be  ${log_dir_files}  ${0}

    # check that the downgrade-backup directory contains the deviceisolation.log file
    ${backup_dir_files} =  List Files In Directory  ${SOPHOS_INSTALL}/plugins/deviceisolation/log/downgrade-backup
    Log  ${backup_dir_files}
    Length Should Be  ${backup_dir_files}  ${1}  downgrade-backup directory contains more than deviceisolation.log
    Should Contain  ${backup_dir_files}  deviceisolation.log

Device Isolation Plugin Installs With Version Ini File
    File Should Exist   ${SOPHOS_INSTALL}/plugins/deviceisolation/VERSION.ini
    VERSION Ini File Contains Proper Format For Product Name   ${SOPHOS_INSTALL}/plugins/deviceisolation/VERSION.ini   SPL-Device-Isolation-Plugin
