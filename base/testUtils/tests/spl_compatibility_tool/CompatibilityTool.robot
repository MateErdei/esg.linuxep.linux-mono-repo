*** Settings ***
Library     OperatingSystem
Library     Process

Library     ${LIBS_DIRECTORY}/CentralUtils.py
Library     ${COMMON_TEST_LIBS}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/WarehouseUtils.py

Resource    ${COMMON_TEST_ROBOT}/McsRouterResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Suite Setup    Compatibility Tool Suite Setup
Suite Teardown    Compatibility Tool Suite Teardown

Test Teardown    Compatibility Tool Test Teardown

# TODO LINUXDAR-4130: Enable for ARM64
Force Tags    TAP_PARALLEL6    EXCLUDE_ARM64

*** Variables ***
${susUrl}    https://sus.sophosupd.com
${cdnUrl}    https://sdds3.sophosupd.com:443

${compatibilityScript}          ${THIN_INSTALLER_INPUT}/SPLCompatibilityChecks.sh
${defaultThinInstallerPath}     ./tmp/thin_installer/thininstaller_files/installer-default.sh
${credentialsPath}              ./tmp/thin_installer/thininstaller_files/credentials.txt

*** Keywords ***
Compatibility Tool Suite Setup
    Regenerate Certificates
    Set_Local_CA_Environment_Variable
    generate_local_ssl_certs_if_they_dont_exist
    Install Local SSL Server Cert To System
    start_local_cloud_server

Compatibility Tool Suite Teardown
    stop_local_cloud_server
    cleanup_local_cloud_server_logs

Compatibility Tool Test Teardown
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
    Should Contain    ${output}    INFO: Server can connect to .com CDN address (${cdnUrl}) directly
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
    Should Be Equal As Integers  ${rc}  ${0}
    Verify Expected Log Lines    ${output}

SPL Compatibility Script Passes With Valid Custom Install Location
    Setup ThinInstaller
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path=${defaultThinInstallerPath} --install-dir=/home
    Should Be Equal As Integers  ${rc}  ${0}
    Verify Expected Log Lines    ${output}

SPL Compatibility Script Fails With Invalid Custom Install Location
    Setup ThinInstaller
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path=${defaultThinInstallerPath} --install-dir=home
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${4}
    Should Contain    ${output}    ERROR: SPL installation will fail, can not install to 'home' because it is a relative path. To install under this directory, please re-run with --install-dir=

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