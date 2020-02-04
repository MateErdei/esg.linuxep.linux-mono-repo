*** Settings ***
Documentation    Tests to verify we can register successfully with
...              fake cloud and save the ID and password we receive.
...              Also tests bad registrations, and deregistrations.

Library     ${LIBS_DIRECTORY}/OSUtils.py
Library     String

Suite Setup      Setup MCS Tests
Suite Teardown   Uninstall SSPL Unless Cleanup Disabled

Resource  ../installer/InstallerResources.robot
Resource  McsRouterResources.robot

Test Setup       Run Keywords
...              Start Local Cloud Server  AND
...              Set Local CA Environment Variable  AND
...              Backup Version Ini

Test Teardown    Run Keywords
...              Stop Local Cloud Server  AND
...              MCSRouter Default Test Teardown  AND
...			     Stop System Watchdog  AND
...              Restore Version Ini

Default Tags  MCS  FAKE_CLOUD  REGISTRATION  MCS_ROUTER

*** Test Case ***
Successful Registration With Correct Log Permissions
    [Tags]  SMOKE  MCS  FAKE_CLOUD  REGISTRATION  MCS_ROUTER
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    File Exists With Permissions  ${SOPHOS_INSTALL}/logs/base/register_central.log  root  root  -rw-------

Successful Registration With New Token Gives New Machine ID
    Start System Watchdog
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  check_mcsenvelope_log_contains  ThisIsAnMCSID+1001

	Register With New Token Local Cloud Server

    # Wait for ThisIsAnMCSID+1002 in MCS logs, showing we've registered again.
    Wait Until Keyword Succeeds
    ...  16 secs
    ...  2 secs
    ...  check_mcsenvelope_log_contains  ThisIsAnMCSID+1002
	
Successful Registration From Different Directories
    Register Different Working Directories   /tmp
    Cleanup Mcsrouter Directories
    Register Different Working Directories   /home
    Cleanup Mcsrouter Directories
    Register Different Working Directories   /
    Cleanup Mcsrouter Directories

Unsuccessful Registration With Bad URL
    [Documentation]  Derived from CLOUD.ERROR.002_Bad_URL.sh
    ${result} =  Fail Register With Junk Address    http://junkaddress:443
    Log   ${result.stdout}
    Log   ${result.stderr}
    Check MCS Config Not Present
    Check Register Central Log Contains  Failed to connect to Sophos Central: Check URL: http://junkaddress:443
    Should Contain   ${result.stdout}    Registering with Sophos Central
    Should Contain   ${result.stderr}    ERROR: Failed to connect to Sophos Central: Check URL: http://junkaddress:443.
    Should Not Contain   ${result.stderr}    Exception
    Should Not Contain   ${result.stderr}    Traceback

Unsuccessful Registration With No certificate set
    Set Environment Variable   MCS_CA   filethatdoesntexist
    Fail To Register With Local Cloud Server  4

Successful Registration, followed by Unsuccessful Registration
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Fail Register With Junk Address    http://junkaddress:443
    Check MCS Config Not Present

Unsuccessful Registration, followed by Successful Registration
    Fail Register With Junk Address    http://junkaddress:443
    Check MCS Config Not Present
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved

Successful Deregistration
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Deregister
    Check Correct MCS Password And ID For Deregister

Deregister Command Stops MCS Router
    Start System Watchdog
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Check MCS Router Running
    Mark Mcsrouter Log
    Deregister
    Wait Until Keyword Succeeds     # using this to ensure mcsrouter shutdown cleanly
    ...  10 secs
    ...  2 secs
    ...  Check Marked Mcsrouter Log Contains    Exiting MCS
    Wait Until Keyword Succeeds     # mcsrouter does not shutdown immediately after sending the exiting message if it dies cleanly
    ...  10 secs
    ...  1 secs
    ...  Check MCS router Not Running

MCSRouter Registers Itself On Startup After Deregistration
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Deregister
    Check Correct MCS Password And ID For Deregister
    Start MCSRouter
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Correct MCS Password And ID For Local Cloud Saved

Verify MCS Router Running After Installation With Registration
    [Tags]  INSTALLER  MCS  FAKE_CLOUD  REGISTRATION   MCS_ROUTER
    Uninstall SSPL
    Check MCS Router Not Running
    Set Local CA Environment Variable
    Run Full Installer  --mcs-url  https://localhost:4443/mcs  --mcs-token   ThisIsARegToken  --allow-override-mcs-ca
    ${pid} =  Check MCS Router Running
    Check Process Running As Sophosspl User And Group  ${pid}
    Stop System Watchdog
    Check MCS Router Not Running

Verify Common Status Contains All Interface MAC Addresses
    [Tags]  MCS  FAKE_CLOUD  MCS_ROUTER  REGISTRATION
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    ${MacAddresses} =  Get Interface Mac Addresses
    :FOR  ${MacAddress}  IN  @{MacAddresses}
    \   Check Register Central Log Contains  <macAddress>${MacAddress}</macAddress>

Successful Registration Sends Current Product Version To Central
    ${StartRegCentralStatus} =  Set Variable  <ns:computerStatus xmlns:ns="http://www.sophos.com/xml/mcs/computerstatus">
    ${EndRegCentralStatus} =  Set Variable  </ns:computerStatus>
    Register With Local Cloud Server
    ${BaseVersion} =  Run Process  grep VERSION ${SOPHOS_INSTALL}/base/VERSION.ini | cut -d' ' -f3  shell=yes
    Check Register Central Log Contains In Order  ${StartRegCentralStatus}  softwareVersion="${BaseVersion.stdout}"  ${EndRegCentralStatus}

Successful Registration Sends Current Product Version 0 If Version Ini File Missing
    ${StartRegCentralStatus} =  Set Variable  <ns:computerStatus xmlns:ns="http://www.sophos.com/xml/mcs/computerstatus">
    ${EndRegCentralStatus} =  Set Variable  </ns:computerStatus>
    Remove File  ${SOPHOS_INSTALL}/base/VERSION.ini
    Register With Local Cloud Server
    Check Register Central Log Contains In Order  ${StartRegCentralStatus}  softwareVersion="0"  ${EndRegCentralStatus}

Successful Registration Sends Current Product Version 0 If Version Ini File Is Missing PRODUCT VERSION
    ${StartRegCentralStatus} =  Set Variable  <ns:computerStatus xmlns:ns="http://www.sophos.com/xml/mcs/computerstatus">
    ${EndRegCentralStatus} =  Set Variable  </ns:computerStatus>
    Remove File  ${SOPHOS_INSTALL}/base/VERSION.ini
    Create File  ${SOPHOS_INSTALL}/base/VERSION.ini  NotAValidVersionFile
    Register With Local Cloud Server
    Check Register Central Log Contains In Order  ${StartRegCentralStatus}  softwareVersion="0"  ${EndRegCentralStatus}

*** Keywords ***
Backup Version Ini
    Copy File  ${SOPHOS_INSTALL}/base/VERSION.ini  /tmp/VERSION.ini.bk

Restore Version Ini
    Move File  /tmp/VERSION.ini.bk  ${SOPHOS_INSTALL}/base/VERSION.ini