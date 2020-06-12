*** Settings ***
Documentation    Demonstrate paused updating is triggered by policy change and the product downloads the correct version from the Warehouse
Default Tags   PAUSED_UPDATE  UPDATE_SCHEDULER  SULDOWNLOADER


Library     ${LIBS_DIRECTORY}/PolicyUtils.py

Resource    ../upgrade_product/UpgradeResources.robot

Suite Setup      Suite Setup
Suite Teardown   Suite Teardown

Test Setup       Test Setup
Test Teardown    Test Teardown


*** Variables ***
${PausedBaseVUTPrevPolicy}    ${GeneratedWarehousePolicies}/base_paused_update_VUT-1.xml
${PausedBaseVUTPolicy}    ${GeneratedWarehousePolicies}/base_paused_update_VUT.xml
${PausedBase999Policy}    ${GeneratedWarehousePolicies}/base_paused_update_999.xml

*** Test Cases ***
Test SSPL Will Updated To A Fixed Version When Paused Updating Is Activated And Updating Will Resolve To Recommended When It Is Deactivated
    [Tags]  PAUSED_UPDATE  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA  EXCLUDE_UBUNTU20

    Start Local Cloud Server  --initial-alc-policy  ${PausedBaseVUTPrevPolicy}

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${PausedBaseVUTPrevPolicy}

    Wait For Initial Update To Fail
    Override LogConf File as Global Level  DEBUG
    # Ensure VUT -1 has successfully installed.
    # waiting for 2nd update success log because the 1st is a guaranteed failure so count starts at 2
    # Updating to VUT -1 using FixedVersion policy
    Perform Update And Check Expected Version Is Installed  ${PausedBaseVUTPrevPolicy}  2


    # Uppgrade to VUT using FixedVersion policy
    Perform Update And Check Expected Version Is Installed    ${PausedBaseVUTPolicy}  3

    # Ensure version does not upgrade to RECOMMENDED when using FixedVersion policy
    Perform Update And Check Expected Version Is Installed    ${PausedBaseVUTPolicy}  3

    # Ensure upgrade to RECOMMENDED when using Non FixedVersion policy
    Perform Update And Check Expected Version Is Installed    ${PausedBase999Policy}  4

*** Keywords ***
Check Expected Version Is Installed
    [Arguments]   ${expectedVersion}
    ${BaseReleaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    Should Contain  ${BaseReleaseVersion}  ${expectedVersion}

Check Update Config Contains Expected Version Value
    [Arguments]   ${expectedVersion}
    ${updateConfigJson} =  Get File  ${SulConfigPath}
    Log File   ${SOPHOS_INSTALL}/base/update/var/update_config.json
    Should Contain  ${updateConfigJson}   fixedVersion": "${expectedVersion}

Perform Update And Check Expected Version Is Installed
    [Arguments]   ${policyFile}  ${updateSuccessLogCount}
    Send ALC Policy And Prepare For Upgrade  ${policyFile}

    # Check installed version matches version from policy
    ${version} =  Get Version From Policy   ${policyFile}

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   1 secs
    ...   Check Update Config Contains Expected Version Value  ${version}

    Trigger Update Now

    Wait Until Keyword Succeeds
    ...   240 secs
    ...   10 secs
    ...   Check MCS Envelope Contains Event Success On N Event Sent  ${updateSuccessLogCount}

    ${length} =    Get Length    ${version}

    ${version} =   Set Variable If
    ...  ${length} > 0  ${version}
    ...  ${length} == 0  99.9.9

    Check Expected Version Is Installed   ${version}
