*** Settings ***
Library     OperatingSystem
Library     Process

Library     ${LIBS_DIRECTORY}/CentralUtils.py
Library     ${LIBS_DIRECTORY}/LogUtils.py
Library     ${LIBS_DIRECTORY}/MCSRouter.py
Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/WarehouseUtils.py

Resource    ../mcs_router/McsRouterResources.robot
Resource    ../upgrade_product/UpgradeResources.robot

Suite Setup    Compatibility Tool Suite Setup
Suite Teardown    Compatibility Tool Suite Teardown

Test Teardown    Compatibility Tool Test Teardown

*** Variables ***
${susUrl}    https://sus.sophosupd.com

${compatibilityScript}          ${SYSTEMPRODUCT_TEST_INPUT}/sspl-thininstaller/SPLCompatibilityChecks.sh
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
    Should Contain            ${output}    INFO: Endpoint can connect to Central directly
    Should Contain            ${output}    INFO: Endpoint can connect to the SUS server (${susUrl}) directly
    Should Contain X Times    ${output}    INFO: Connection verified to CDN server, endpoint is able to download    ${6}

    Should Not Contain    ${output}    ERROR:
    Should Not Contain    ${output}    WARN:

*** Test Case ***
SPL Compatibility Script Passes Without Path To ThinInstaller
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript}
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${0}

SPL Compatibility Script Passes
    Setup ThinInstaller
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path ${defaultThinInstallerPath}
    Should Be Equal As Integers  ${rc}  ${0}
    Verify Expected Log Lines    ${output}

SPL Compatibility Script Passes With Valid Custom Install Location
    Setup ThinInstaller
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path ${defaultThinInstallerPath} --install-dir /home
    Should Be Equal As Integers  ${rc}  ${0}
    Verify Expected Log Lines    ${output}

SPL Compatibility Script Fails With Invalid Custom Install Location
    Setup ThinInstaller
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path ${defaultThinInstallerPath} --install-dir home
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${1}
    Should Contain    ${output}    ERROR: Can not install to 'home' because it is a relative path.

SPL Compatibility Script Passes but Logs Warning With Broken Message Relays
    Setup ThinInstaller    dummyhost1:10000,1,2;localhost:20000,2,4    ${None}
    
    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path ${defaultThinInstallerPath}
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${0}

    Should Contain    ${output}    WARN: SPL installation will not be performed via dummyhost1:10000. Endpoint cannot connect to Central via dummyhost1:10000
    Should Contain    ${output}    WARN: SPL installation will not be performed via localhost:20000. Endpoint cannot connect to Central via localhost:20000

    Should Contain    ${output}    WARN: SPL installation will not be performed via dummyhost1:10000. Endpoint cannot connect to the SUS server (${susUrl}) via dummyhost1:10000
    Should Contain    ${output}    WARN: SPL installation will not be performed via localhost:20000. Endpoint cannot connect to the SUS server (${susUrl}) via localhost:20000

    Should Contain    ${output}    WARN: SPL installation will not be performed via localhost:20000. Failed to get supplement from CDN server
    Should Contain    ${output}    WARN: SPL installation will not be performed via localhost:20000. Failed to get supplement from CDN server

SPL Compatibility Script Passes but Logs Warning With Broken Update Caches
    Setup ThinInstaller    ${None}    localhost:8080,2,1;localhost:1235,1,1

    ${rc}   ${output} =    Run And Return Rc And Output    /bin/bash ${compatibilityScript} --installer-path ${defaultThinInstallerPath}
    Log    ${output}
    Should Be Equal As Integers  ${rc}  ${0}

    Should Contain    ${output}    WARN: Endpoint cannot connect to configured Update Cache (localhost:8080)
    Should Contain    ${output}    WARN: Endpoint cannot connect to configured Update Cache (localhost:1235)