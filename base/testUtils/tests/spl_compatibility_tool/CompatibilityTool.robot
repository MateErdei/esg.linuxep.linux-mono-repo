*** Settings ***
Library     OperatingSystem
Library     Process

Library     ${COMMON_TEST_LIBS}/CentralUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${COMMON_TEST_LIBS}/MCSRouter.py
Library     ${COMMON_TEST_LIBS}/ThinInstallerUtils.py
Library     ${COMMON_TEST_LIBS}/WarehouseUtils.py

Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Suite Setup    Compatibility Tool Suite Setup
Suite Teardown    Compatibility Tool Suite Teardown

Test Teardown    Compatibility Tool Test Teardown

Force Tags    TAP_PARALLEL6

*** Variables ***
${susUrl}    https://sus.sophosupd.com
${cdnUrl}    https://sdds3.sophosupd.com:443

${compatibilityScript}          ${THIN_INSTALLER_INPUT}/SPLCompatibilityChecks.sh
${defaultThinInstallerPath}     ./tmp/thin_installer/thininstaller_files/installer-default.sh
${credentialsPath}              ./tmp/thin_installer/thininstaller_files/credentials.txt

*** Keywords ***
Compatibility Tool Suite Setup
    Setup_MCS_Cert_Override
    start_local_cloud_server

Compatibility Tool Suite Teardown
    stop_local_cloud_server
    cleanup_local_cloud_server_logs

Compatibility Tool Test Teardown
    OnFail.run_teardown_functions
    Remove File    ${defaultThinInstallerPath}
    Remove File    ${credentialsPath}

Setup ThinInstaller
    [Arguments]    ${messageRelays}=${None}    ${updateCaches}=${None}
    get_thininstaller
    create_default_credentials_file    message_relays=${messageRelays}    update_caches=${updateCaches}
    build_default_creds_thininstaller_from_sections

Verify Expected Log Lines
    [Arguments]    ${output}
    Should Contain    ${output}    INFO: Server can connect to Sophos Central directly
    Should Contain    ${output}    INFO: Server can connect to the SUS server (${susUrl}) directly
    Should Contain    ${output}    INFO: Server can connect to CDN address (${cdnUrl}) directly
    Should Contain    ${output}    INFO: Connection verified to CDN server, server was able to download all SPL packages directly
    Should Contain    ${output}    SPL can be installed on this system based on the pre-installation checks

    Should Not Contain    ${output}    ERROR:
    Should Not Contain    ${output}    WARN:

*** Test Case ***
SPL Compatibility Script Passes Without Path To ThinInstaller
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript}
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${0}

SPL Compatibility Script Passes
    Setup ThinInstaller
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path=${defaultThinInstallerPath}
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${0}
    Verify Expected Log Lines    ${output}

SPL Compatibility Script Passes With Valid Custom Install Location
    Setup ThinInstaller
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path=${defaultThinInstallerPath} --install-dir=/home
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${0}
    Verify Expected Log Lines    ${output}

SPL Compatibility Script Fails With Invalid Custom Install Location
    Setup ThinInstaller
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path=${defaultThinInstallerPath} --install-dir=home
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${4}
    Should Contain    ${output}    ERROR: SPL installation will fail, can not install to 'home' because it is a relative path. To install under this directory, please re-run with --install-dir=

SPL Compatibility Script Fails With Invalid Custom Install Location when Location Is A File
    Setup ThinInstaller
    Create Directory     /testInstall
    Create File     /testInstall/file
    Register Cleanup  Remove Directory    /testInstall    true
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path=${defaultThinInstallerPath} --install-dir=/testInstall/file
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${4}
    Should Contain    ${output}    ERROR: SPL installation will fail, can not install to '/testInstall/file' as it is a file

SPL Compatibility Script Passes but Logs Warning With Broken Message Relays
    Setup ThinInstaller    dummyhost1:10000,1,2;localhost:20000,2,4    ${None}
    
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path=${defaultThinInstallerPath}
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${2}

    Should Contain    ${output}    WARN: SPL installation will not be performed via dummyhost1:10000. Server cannot connect to Sophos Central via dummyhost1:10000
    Should Contain    ${output}    WARN: SPL installation will not be performed via localhost:20000. Server cannot connect to Sophos Central via localhost:20000

    Should Contain    ${output}    WARN: SPL installation will not be performed via dummyhost1:10000. Server cannot connect to the SUS server (${susUrl}) via dummyhost1:10000
    Should Contain    ${output}    WARN: SPL installation will not be performed via localhost:20000. Server cannot connect to the SUS server (${susUrl}) via localhost:20000

SPL Compatibility Script Passes but Logs Warning With Broken Update Caches
    Setup ThinInstaller    ${None}    localhost:8080,2,1;localhost:1235,1,1

    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path=${defaultThinInstallerPath}
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${2}

    Should Contain    ${output}    WARN: Server cannot connect to configured Update Cache (localhost:8080)
    Should Contain    ${output}    WARN: Server cannot connect to configured Update Cache (localhost:1235)