*** Settings ***
Documentation    EndToEnd Test Related to MTR Features

Library           OperatingSystem

Library     ${LIBS_DIRECTORY}/WarehouseUtils.py
Library     ${LIBS_DIRECTORY}/OSUtils.py

Resource    ../installer/InstallerResources.robot
Resource    ../thin_installer/ThinInstallerResources.robot
Resource    ../GeneralTeardownResource.robot
Resource    ../mcs_router/McsRouterResources.robot
Resource    ../mdr_plugin/MDRResources.robot
Resource    ../mcs_router-nova/McsRouterNovaResources.robot
Resource    ../upgrade_product/UpgradeResources.robot
Resource  ../telemetry/TelemetryResources.robot


Library     ${LIBS_DIRECTORY}/ThinInstallerUtils.py
Library     ${LIBS_DIRECTORY}/MTRService.py
Library     ${LIBS_DIRECTORY}/FullInstallerUtils.py

Suite Setup     MDR Install And Upgrade Setup
Suite Teardown  MDR Install And Upgrade Teardown

Test Setup     Test Setup MDR Install And Upgrade
Test Teardown  Test Teardown MDR Install And Upgrade

Default Tags  MDR_REGRESSION_TESTS


*** Variables ***
${PreviousALC}                  /tmp/previousALC.xml
${NextALC}                      /tmp/nextALC.xml
${ALC_POLICY_PATH}              ${SOPHOS_INSTALL}/base/mcs/policy/ALC-1_policy.xml
${SULDOWNLOADER_LOG_PATH}       ${SOPHOS_INSTALL}/logs/base/suldownloader.log
${TEST_MDR_POLICY_PATH}         ${SUPPORT_FILES}/CentralXml/MDR_policy_with_correct_url.xml
${PROD_CERTS}                   ${SUPPORT_FILES}/sophos_certs/prod_certs


*** Test Cases ***

Install Release Candidate And Basic Services are Working
    [Timeout]   10 minutes
    Start Local Cloud Server   --initial-alc-policy=${NextALC}   --initial-mdr-policy  ${TEST_MDR_POLICY_PATH}
    Log To Console   "Release Candidate Credential: ${ReleaseCandidateCredential}"
    Send Policy File  alc  ${NextALC}
    Configure And Run Thininstaller Using Real Warehouse Policy  0   ${NextALC}   override_certs_dir=${PROD_CERTS}
    Wait For MTR to be Installed   ${NextALC}

    VERSION Ini File Contains Proper Format For Product Name   ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini   Sophos Managed Threat Response plug-in

    Check Status And Event Message Sent

    Check Basic MTR Features Work As Expected

    Verify Status Message Is Sent Again After Re-registration

Upgrade to Release Candidate And Basic Services are Working
    [Timeout]   10 minutes
    Start Local Cloud Server   --initial-alc-policy=${PreviousALC}  --initial-mdr-policy  ${TEST_MDR_POLICY_PATH}
    Log To Console   "Previous Released version: ${PreviousReleaseCredential}"
    Send Policy File  alc  ${PreviousALC}
    Configure And Run Thininstaller Using Real Warehouse Policy  0   ${PreviousALC}    override_certs_dir=${PROD_CERTS}
    Wait For MTR to be Installed   ${PreviousALC}
    Check Osquery Is Sending Scheduled Query Results
    Copy File   ${SOPHOS_INSTALL}/base/VERSION.ini  /tmp/BASEVERSION.ini
    Copy File   ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini  /tmp/MTRVERSION.ini

    # upgrade:
    Remove File    ${SULDOWNLOADER_LOG_PATH}
    Send And Wait for Policy  ${NextALC}
    Trigger Update Now
    Wait For Update To be Executed

    @{previous}  Create List  /tmp/BASEVERSION.ini  /tmp/MTRVERSION.ini
    @{latest}  Create List   ${SOPHOS_INSTALL}/base/VERSION.ini  ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini
    Check Version Files Report A Valid Upgrade   ${previous}  ${latest}

    Check Basic MTR Features Work As Expected


*** Keywords ***
Files Should Not Match
    [Arguments]  ${path1}  ${expected2}

    ${current_content} =  Get File  ${path1}
    ${expected_content} =  Get File  ${expected2}
    Should Not Be Equal  ${expected_content}  ${current_content}   msg="Files content are the same"


Files Should Match
    [Arguments]  ${path1}  ${expected2}

    ${current_content} =  Get File  ${path1}
    ${expected_content} =  Get File  ${expected2}
    Should Be Equal  ${expected_content}  ${current_content}   msg="Files content are different"


Wait for Policy
    [Arguments]  ${expected_alc_policy_path}
    Wait Until Keyword Succeeds
        ...  30 secs
        ...  5 secs
        ...  ALC Policy Matches  ${expected_alc_policy_path}


Send And Wait for Policy
    [Arguments]  ${expected_alc_policy_path}
    Wait Until Keyword Succeeds
    ...  3x
    ...  10 secs
    ...  Run Keywords  Send Policy File  alc  ${expected_alc_policy_path}  AND
         ...   Wait for Policy  ${expected_alc_policy_path}

Wait For Update To be Executed
    Wait For Update Action To Be Processed
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  10 secs
    ...  SulDownloader Report Success
    Wait MDR Policy And Status Is Reported
    Wait For MDR To Report Certificate Issue Fix and Restart


Wait For MTR to be Installed
    [Arguments]  ${expected_alc_policy_path}
    Wait for Policy  ${expected_alc_policy_path}
    Wait For Update To be Executed

ALC Policy Matches
    [Arguments]  ${expected_alc_policy_path}
    Files Should Match   ${ALC_POLICY_PATH}  ${expected_alc_policy_path}

SulDownloader Report Success
    ${log} =  Get File  ${SULDOWNLOADER_LOG_PATH}
    Should Contain  ${log}  Update success
    Should Not Contain  ${log}  Failed to connect to the warehouse

Wait MDR Policy And Status Is Reported
    Wait until MDR policy is Downloaded
    Wait For MDR Status
    ${MDRStatus} =  Get File  ${MDR_STATUS_XML}
    Should Contain  ${MDRStatus}   Res='Same'
    Should Not Contain  ${MDRStatus}   RevID=''

Wait For MDR To Report Certificate Issue Fix and Restart
    Wait For MDR To Report Certificate Issue
    Fix DBOS Certs And Restart

Wait For MDR To Report Certificate Issue
    Wait Until Keyword Succeeds
    ...  200 secs
    ...  10 secs
    ...  MDR Reports Issues

MDR Reports Issues
    ${count} =  Count Dbos Log Errors Default Ignore
    Should Not Be Equal as Integers   ${count}  0


MDR Install And Upgrade Setup
    ${placeholder} =  Get Environment Variable  PreviousReleaseCredential  default=notdefined
    Should Not Be Equal  ${placeholder}  notdefined   This test requires definition of the environment variable PreviousReleaseCredential. Example: CSP200106101243.
    Set Suite Variable  ${PreviousReleaseCredential}  ${placeholder}

    ${placeholder} =  Get Environment Variable  ReleaseCandidateCredential  default=notdefined
    Should Not Be Equal  ${placeholder}  notdefined   This test requires definition of the environment variable ReleaseCandidateCredential. Example: CSP200106101243.
    Set Suite Variable  ${ReleaseCandidateCredential}  ${placeholder}

    Create ALC Policy For Warehouse Credentials  base_and_mtr_VUT.xml   ${PreviousReleaseCredential}      ${PreviousALC}
    Create ALC Policy For Warehouse Credentials  base_and_mtr_VUT.xml   ${ReleaseCandidateCredential}     ${NextALC}
    File Should Exist  ${PreviousALC}
    File Should Exist  ${NextALC}

    Setup etc hosts To Connect To Internal Warehouse
    Install Internal Warehouse Certs

    ${result} =  Run Process  curl -v https://dci.sophosupd.com/cloudupdate/   shell=True
    Should Be Equal As Integers  ${result.rc}  0  "Failed to Verify connection to Update Server. Please, check endpoint is configured. (Hint: tools/setup_sspl/setupEnvironment2.sh).\nStdOut: ${result.stdout}\n StdErr: ${result.stderr}"

    Regenerate Certificates
    Setup MCS Tests

MDR Install And Upgrade Teardown
    Run Keyword and Ignore Error  Remove File  ${PreviousALC}
    Run Keyword and Ignore Error  Remove File  ${NextALC}
    Revert System CA Certs
    Clear etc hosts Of Entries To Connect To Internal Warehouse

Test Setup MDR Install And Upgrade
    Require Uninstalled
    Verify Group Removed
    Verify User Removed
    Should Not Exist   ${SOPHOS_INSTALL}
    Start MTRService

Test Teardown MDR Install And Upgrade
    Run Keyword If Test Failed    Log File   ${PreviousALC}
    Run Keyword If Test Failed    Log File   ${NextALC}
    Run Keyword if Test Failed    Log File  ${ALC_POLICY_PATH}
    Run Keyword if Test Failed    Log File  ${UPDATE_CONFIG}
    Run Keyword if Test Failed    Log File  ${SOPHOS_INSTALL}/base/VERSION.ini
    Run Keyword if Test Failed    Log File  ${SOPHOS_INSTALL}/plugins/mtr/VERSION.ini
    Stop Local Cloud Server
    Stop MTRService
    Run Keyword If Test Failed   Dump MTR Service Logs
    MCSRouter Default Test Teardown


Check Basic MTR Features Work As Expected
    Check Osquery Is Sending Scheduled Query Results
    Check MTRConsole Can Send Action    ls /tmp
    Check MTRConsole Can Delete File
    Check Osquery Respond To LiveQuery
    Check No Issue Reported
    Check MTRConsole Can Trigger LiveTerminal


Check No Issue Reported
    Check Log Does Not Contain  ERROR  ${SULDOWNLOADER_LOG_PATH}  suldownloader
    Check Log Does Not Contain  ERROR  ${MDR_LOG_FILE}  mtr
    Check Log Does Not Contain  WARN  ${MDR_LOG_FILE}  mtr
    ${count} =  Count Dbos Log Errors Default Ignore
    Should Be Equal as Integers   ${count}  0

    ${result} =  Run Process    systemctl  status  sophos-spl
    Should Contain  ${result.stdout}  active
    Should Contain  ${result.stdout}  sophos_watchdog
    Should Contain  ${result.stdout}  python3
    Should Contain  ${result.stdout}  sophos_management
    Should Contain  ${result.stdout}  UpdateScheduler
    Should Contain  ${result.stdout}  SophosMTR
    Should Contain  ${result.stdout}  osquery
    # watchdog and agent
    Running Processes Should Match the Count   osquery  ${SOPHOS_INSTALL}/plugins/mtr  2
    Running Processes Should Match the Count   SophosMTR  ${SOPHOS_INSTALL}/plugins/mtr  1

    Mark Expected Error In Log  ${SOPHOS_INSTALL}/logs/base/wdctl.log  wdctlActions <> Plugin "mtr" not in registry

    Check All Product Logs Do Not Contain Error


Check Status And Event Message Sent
    Check Cloud Server Log Contains   <appInfo><number>0</number><updateSource>Sophos</updateSource>  1
    Wait Until Keyword Succeeds
    ...  50 secs
    ...  10 secs
    ...  Check Cloud Server Log Contains   www.sophos.com/xml/mansys/AutoUpdateStatus  1
    Check Cloud Server Log Contains   ServerProtectionLinux-MDR-DBOS-Component  1
    Check Cloud Server Log Contains   Sophos Query Component  1



Verify Status Message Is Sent Again After Re-registration
    Log File  ${BASE_LOGS_DIR}/sophosspl/mcs_envelope.log
    Remove File  ${BASE_LOGS_DIR}/sophosspl/mcs_envelope.log

    #Register again and wait for ThisIsAnMCSID+1002 in MCS logs, showing we've registered again.
    List Files In Directory 	${UPDATE_DIR}/var 	report*.json
    Register With New Token Local Cloud Server
    List Files In Directory 	${UPDATE_DIR}/var 	report*.json

    #Wait until the following message appears in the mcs envelope as it marks when the registration is
    #complete and all plugins have policies.
    Wait Until Keyword Succeeds
    ...  2 min
    ...  10 secs
    ...  Check MCSenvelope Log Contains  PUT /statuses/endpoint/ThisIsAnMCSID+1002

    # Force an update so that we don't have to wait 5 to 10 mins
    Trigger Update Now

    # verify that a new alc status is sent
    Wait Until Keyword Succeeds
    ...  2 min
    ...  2 secs
    ...  Should Exist    ${MCS_DIR}/status/ALC_status.xml

    Wait Until Keyword Succeeds
    ...  1 min
    ...  2 secs
    ...  Check MCSenvelope Log Contains   <event><appId>ALC</appId>



