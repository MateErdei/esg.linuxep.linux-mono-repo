*** Settings ***
Documentation    Demonstrate paused updating is triggered by policy change and the product downloads the correct version from the Warehouse
Default Tags   PAUSED_UPDATE  UPDATE_SCHEDULER  SULDOWNLOADER


Library     ${LIBS_DIRECTORY}/PolicyUtils.py

Resource    ../upgrade_product/UpgradeResources.robot

Suite Setup      Suite Setup
Suite Teardown   Suite Teardown

Test Setup       Test Setup
Test Teardown    Test Teardown

Force Tags  LOAD7
*** Variables ***
${BaseOnlyVUTPolicy}                  ${GeneratedWarehousePolicies}/base_only_VUT.xml
${BaseOnlyFixedVersionVUTPolicy}      ${GeneratedWarehousePolicies}/base_only_fixed_version_VUT.xml
${BaseOnlyFixedVersion999Policy}      ${GeneratedWarehousePolicies}/base_only_fixed_version_999.xml

*** Test Cases ***

Test SSPL Will Updated To A Fixed Version When Paused Updating Is Activated And Updating Will Resolve To Recommended When It Is Deactivated
    [Timeout]  10mins
    [Tags]  PAUSED_UPDATE  INSTALLER  THIN_INSTALLER  UNINSTALL  UPDATE_SCHEDULER  SULDOWNLOADER  OSTIA  EXCLUDE_UBUNTU20    TESTFAILURE
    [Teardown]  Test Teardown And Replace Policy
    Start Local Cloud Server  --initial-alc-policy  ${BaseOnlyVUTPolicy}  --initial-flags  ${SUPPORT_FILES}/CentralXml/FLAGS_sdds2.json

    Configure And Run Thininstaller Using Real Warehouse Policy  0  ${BaseOnlyVUTPolicy}

    Override Local LogConf File for a component   DEBUG  global

    Run Process  systemctl  restart  sophos-spl

    # update via fixed version 999 which does not exist in the given warehouse so expecting this to fail the update
    # this is to prove the product is updating based in fixed version settings.
    Perform Failed Update And Check Log for Expected Error  ${BaseOnlyFixedVersion999Policy}  generatingReportLogCount=2  updateResultOccurenceLogCount=1  updateResult=Product missing from warehouse: ServerProtectionLinux-Base

    # update using fixed version
    Log File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    # clear log
    Remove File   ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    # update via fixed version
    Copy File  ${BaseOnlyFixedVersionVUTPolicy}  ${BaseOnlyFixedVersionVUTPolicy}.bak
    ${ExpectedBaseSuiteDevVersion} =  get_version_for_rigidname_in_vut_warehouse   ServerProtectionLinux-Base
    replace_string_in_file  %SUITVERSION%  \"${ExpectedBaseSuiteDevVersion}\"  ${BaseOnlyFixedVersionVUTPolicy}
    Perform Update And Check Expected Version Is Installed  ${BaseOnlyFixedVersionVUTPolicy}  1

    # update using release tag.
    Log File  ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    # clear log
    Remove File   ${SOPHOS_INSTALL}/logs/base/suldownloader.log
    # update via fixed version
    Perform Update And Check Expected Version Is Installed  ${BaseOnlyVUTPolicy}   1


*** Keywords ***
Check Expected Version Is Installed
    [Arguments]   ${expectedVersion}
    ${BaseReleaseVersion} =     Get Version Number From Ini File   ${InstalledBaseVersionFile}
    Should Contain  ${BaseReleaseVersion}  ${expectedVersion}

Check Update Config Contains Expected Version Value
    [Arguments]   ${expectedVersion}
    ${updateConfigJson} =  Get File  ${UPDATE_CONFIG}
    Log File   ${UPDATE_CONFIG}
    Should Contain  ${updateConfigJson}   fixedVersion": "${expectedVersion}

Perform Update And Check Expected Version Is Installed
    [Arguments]   ${policyFile}  ${updateSuccessLogCount}  ${updateResult}=Update success
    Send ALC Policy And Prepare For Upgrade  ${policyFile}

    # Check installed version matches version from policy
    ${version} =  Get Version From Policy   ${policyFile}

    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check Update Config Contains Expected Version Value  ${version}


    Wait Until Keyword Succeeds
    ...   240 secs
    ...   10 secs
    ...   Check SulDownloader Log Contains String N Times   ${updateResult}   ${updateSuccessLogCount}

    #  This is to make sure the update has finished in case the previous report check is does not match a string that is produced at end of the update process.
    Wait Until Keyword Succeeds
    ...   240 secs
    ...   2 secs
    ...   Check SulDownloader Log Contains String N Times   Generating the report file   ${updateSuccessLogCount}

Perform Failed Update And Check Log for Expected Error
    [Arguments]   ${policyFile}  ${generatingReportLogCount}  ${updateResultOccurenceLogCount}    ${updateResult}=Update success
    Send ALC Policy And Prepare For Upgrade  ${policyFile}

    # Check installed version matches version from policy
    ${version} =  Get Version From Policy   ${policyFile}

    Wait Until Keyword Succeeds
    ...   120 secs
    ...   10 secs
    ...   Check Update Config Contains Expected Version Value  ${version}

    Wait Until Keyword Succeeds
    ...   30 secs
    ...   2 secs
    ...   Check SulDownloader Log Contains String N Times   ${updateResult}   ${updateResultOccurenceLogCount}

    #  This is to make sure the update has finished in case the previous report check is does not match a string that is produced at end of the update process.
    Wait Until Keyword Succeeds
    ...   240 secs
    ...   2 secs
    ...   Check SulDownloader Log Contains String N Times   Generating the report file   ${generatingReportLogCount}

Test Teardown And Replace Policy
    Test Teardown
    Move File  ${BaseOnlyFixedVersionVUTPolicy}.bak  ${BaseOnlyFixedVersionVUTPolicy}