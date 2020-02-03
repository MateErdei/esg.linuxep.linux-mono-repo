#run from command line: <path>/everest-systemproducttests$ sudo python2.7 -m  robot --suite "InstallPlugin" .

*** Settings ***
Test Setup  Install Example Plugin Test Setup
Test Teardown    Install Plugin Test Teardown

Library     ${libs_directory}/WarehouseGenerator.py
Library     ${libs_directory}/UpdateServer.py
Library     ${libs_directory}/SulDownloader.py
Library     ${libs_directory}/LogUtils.py
Library     ${libs_directory}/FakeManagement.py

Resource  ExamplePluginResources.robot
Resource  ../GeneralTeardownResource.robot
Resource  ../watchdog/LogControlResources.robot

Default Tags  INSTALLER  EXAMPLE_PLUGIN

*** Test Cases ***
Plugin Installs Correctly From Warehouse
    File Should Exist    ${SOPHOS_INSTALL}/plugins/Example/bin/example
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Example Plugin Running

Verify Fake Management
    Stop System Watchdog And Wait
    Set Environment Variable  PATHTOSCAN  ${tmpdir}/scanpath
    Start Fake Management
    Set Log Level For Component And Reset and Return Previous Log  Example  DEBUG
    Link AppId Plugin  SAV  Example
    Start Process      ${SOPHOS_INSTALL}/plugins/Example/bin/example
    Wait Registered
    Check Status Contains  <CompRes xmlns="com.sophos\\msys\\csc" Res="NoRef" RevID="" policyType="2" />
    Send Policy        SAVPolicyOnAccessEnabled
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check Status Compliant
    Send Action        ScanNowAction
    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  Check Telemetry Contain   "Number of Scans" : 1
    Create File        ${tmpdir}/scanpath/newvirus.txt   "this contain VIRUS threat"
    File Should Exist  ${tmpdir}/scanpath/newvirus.txt
    Send Action        ScanNowAction
    Wait Until Keyword Succeeds
    ...  3 secs
    ...  1 secs
    ...  Check Telemetry Contain   "Number of Infections" : 1
    Check Example Plugin Log Contains  Example <> Plugin Callback Started  Example Plugin did not log that its Callback started
    Check Example Plugin Log Contains  pluginapi <> Plugin initialized  Example Plugin did not log pluginapi logs

    Stop Fake Management

*** Keywords ***
Install Plugin Test Teardown
    General Test Teardown
    Terminate All Processes  kill=True
    Remove Directory  ${tmpdir}  recursive=true

