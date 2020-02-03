*** Settings ***
Documentation    Check UpdateScheduler Plugin using real SulDownloader and the Fake Warehouse to prove it works.

Test Setup      Setup For Test
Test Teardown   Teardown For Test

Library    Process
Library    OperatingSystem
Library    ${libs_directory}/UpdateSchedulerHelper.py
Library    ${libs_directory}/UpdateServer.py
Library    ${libs_directory}/WarehouseGenerator.py
Library    ${libs_directory}/LogUtils.py
Library    ${libs_directory}/FullInstallerUtils.py

Resource  SchedulerUpdateResources.robot

*** Variables ***
${CustomPath}       /CustomPath
${BasePolicy}       ${SUPPORT_FILES}/CentralXml/RealWarehousePolicies/GeneratedAlcPolicies/base_only_VUT.xml

*** Test Cases ***
UpdateScheduler Updates Against the Fake Warehouse In Custom Install Directory
    [Tags]  UPDATE_SCHEDULER  CUSTOM_LOCATION
    Should Exist  ${CustomPath}/sophos-spl

    Should Not Exist  /opt/sophos-spl

    Send Policy With No Cache And No Proxy
    Replace Sophos URLS to Localhost
    Simulate Update Now

    ${eventPath} =  Check Status and Events Are Created  waitTime=120 secs

    Log File  ${CustomPath}/sophos-spl/logs/base/suldownloader.log

    Check Event Report Success  ${eventPath}

    Should Exist  ${CustomPath}/sophos-spl
    Should Not Exist  /opt/sophos-spl


*** Keywords ***
Setup For Test
    Require Uninstalled
    Should Not Exist  /opt/sophos-spl
    Set Sophos Install Environment Variable  ${CustomPath}/sophos-spl
    Set Suite Variable  ${SOPHOS_INSTALL_CACHE}  ${SOPHOS_INSTALL}
    Set Global Variable  ${SOPHOS_INSTALL}  ${CustomPath}/sophos-spl
    Setup Servers For Update Scheduler
    Create Directory    ${tmpdir}

Teardown For Test
    Teardown Servers For Update Scheduler
    Require Uninstalled
    Remove Directory   ${CustomPath}    recursive=True
    Set Global Variable  ${SOPHOS_INSTALL}  ${SOPHOS_INSTALL_CACHE}
    Log  ${SOPHOS_INSTALL_ENVIRONMENT_CACHE}
    Reset Sophos Install Environment Variable


