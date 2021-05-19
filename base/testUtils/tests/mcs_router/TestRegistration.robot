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
Force Tags  LOAD3

*** Test Case ***
Successful Register With Cloud And Correct Status Is Sent Up In Amazon
    [Documentation]  Derived from CLOUD.001_Register_in_cloud.sh
    [Tags]  AMAZON_LINUX  MCS  MCS_ROUTER  FAKE_CLOUD
    Register With Local Cloud Server
    Check Register Central Log Contains In Order   <aws>  <region>  </region>  <accountId>  </accountId>  <instanceId>  </instanceId>  </aws>

Successful Registration With Correct Log Permissions
    [Tags]  SMOKE  MCS  FAKE_CLOUD  REGISTRATION  MCS_ROUTER  TAP_TESTS
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Cloud Server Log Does Not Contain  Processing deployment api call
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken
    File Exists With Permissions  ${SOPHOS_INSTALL}/logs/base/register_central.log  root  root  -rw-------

Successful Registration With New Token Gives New Machine ID and resends existing status and event
    Start System Watchdog
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  check_mcsenvelope_log_contains  ThisIsAnMCSID+1001

    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  check_marked_mcsrouter_log_contains_string_n_times    Sending status for ALC adapter  1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  check_marked_mcsrouter_log_contains_string_n_times    queuing event for ALC  1
    Mark mcsrouter log
	Register With New Token Local Cloud Server

    # Wait for ThisIsAnMCSID+1002 in MCS logs, showing we've registered again.
    Wait Until Keyword Succeeds
    ...  16 secs
    ...  2 secs
    ...  check_mcsenvelope_log_contains  ThisIsAnMCSID+1002
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  check_marked_mcsrouter_log_contains_string_n_times    Sending status for ALC adapter  1
    Wait Until Keyword Succeeds
    ...  1 min
    ...  5 secs
    ...  check_marked_mcsrouter_log_contains_string_n_times    queuing event for ALC  1
Successful Re-Registration When MCSID is Missing
    Start System Watchdog
    Register With Local Cloud Server
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  check_mcsenvelope_log_contains  ThisIsAnMCSID+1001

    Remove File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    Create File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    ${result} =  Run Process  chown  sophos-spl-local:sophos-spl-group  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    Kill Mcsrouter
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check MCS Router Not Running

    Wait Until Keyword Succeeds
    ...  40 secs
    ...  5 secs
    ...  Check MCS Router Log Contains In Order
            ...      Re-registering with MCS
            ...      Successfully directly connected

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

Register Fails with bad token
    [Documentation]  Derived from CLOUD.ERROR.003_Bad_token.sh
    Fail Register With Junk Token  badtoken

    Wait Until Keyword Succeeds
    ...  10 secs
    ...  1 secs
    ...  Check MCS Router Not Running

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
    Check Process Running As Sophosspl Local And Group  ${pid}
    Stop System Watchdog
    Check MCS Router Not Running

Verify Common Status Contains All Interface MAC Addresses
    [Tags]  MCS  FAKE_CLOUD  MCS_ROUTER  REGISTRATION
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    ${MacAddresses} =  Get Interface Mac Addresses
    FOR  ${MacAddress}  IN  @{MacAddresses}
    Check Register Central Log Contains  <macAddress>${MacAddress}</macAddress>
    END

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

Successful Register With Group Specified
    Create File  ${ETC_DIR}/install_options   --group=A Group
    Register With Local Cloud Server
    Check Register Central Log Contains  <deviceGroup>A Group</deviceGroup>

Registering With Wrong Group Command Does Not Add To Group
    Create File  ${ETC_DIR}/install_options   --group
    Register With Local Cloud Server
    Check Register Central Log Does Not Contain  <deviceGroup>--group</deviceGroup>

Registering With Unicode Group Adds To Group
    Create File  ${ETC_DIR}/install_options   --group=ümlaut
    Register With Local Cloud Server
    Check Register Central Log Contains  <deviceGroup>ümlaut</deviceGroup>

Registering And Asking For Just MDR
    Register With Local Cloud Server  customer_token=ThisIsACustomerToken  product_selection=mdr
    Check Cloud Server Log Contains  products requested from deployment API: ['mdr']
    Check Cloud Server Log Contains  Register with ::ThisIsARegTokenFromTheDeploymentAPI

Registering And Asking For Just Antivirus
    Register With Local Cloud Server  customer_token=ThisIsACustomerToken  product_selection=antivirus
    Check Cloud Server Log Contains  products requested from deployment API: ['antivirus']
    Check Cloud Server Log Contains  Register with ::ThisIsARegTokenFromTheDeploymentAPI

Registering And Asking For MDR And Antivirus
    Register With Local Cloud Server  customer_token=ThisIsACustomerToken  product_selection=mdr,antivirus
    Check Cloud Server Log Contains  products requested from deployment API: ['mdr', 'antivirus']
    Check Cloud Server Log Contains  Register with ::ThisIsARegTokenFromTheDeploymentAPI

Registering And Asking For Unsupported Product
    Register With Local Cloud Server  customer_token=ThisIsACustomerToken  product_selection=unsupported_product
    Check Register Central Log Contains  requested product: UNSUPPORTED_PRODUCT, is not supported for this licence and platform. Reasons: ['UNSUPPORTED_PLATFORM']. The product will not be assigned in central
    Check Cloud Server Log Contains  products requested from deployment API: ['unsupported_product']
    Check Cloud Server Log Contains  Register with ::ThisIsARegTokenFromTheDeploymentAPI

Register Central Filters Products With Unsafe XML Characters And Logs Warning
    Register With Local Cloud Server  customer_token=ThisIsACustomerToken  product_selection=mdr,unsupported_product>
    Check Register Central Log Contains  Product string: unsupported_product> is not safe to use as part xml request body
    Check Cloud Server Log Contains  products requested from deployment API: ['mdr']
    Check Cloud Server Log Contains  Register with ::ThisIsARegTokenFromTheDeploymentAPI

Registering And Asking For No Products
    Register With Local Cloud Server  customer_token=ThisIsACustomerToken  product_selection=none
    Check Cloud Server Log Contains  products requested from deployment API: []
    Check Cloud Server Log Contains  Register with ::ThisIsARegTokenFromTheDeploymentAPI

Registering Where Deployment API Check Fails with 400 Falls Back To Default Registration Token
    Register With Local Cloud Server  customer_token=ThisIsACustomerToken  product_selection=fail_400
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken
    Check Register Central Log Contains  Deployment API call failed: MCS HTTP Error: code=400
    Check Register Central Log Contains  Continuing registration with default registration token: ThisIsARegToken

Registering Where Deployment API Check Fails with 401 Falls Back To Default Registration Token
    Register With Local Cloud Server  customer_token=ThisIsACustomerToken  product_selection=fail_401
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken
    Check Register Central Log Contains  Deployment API call failed: MCS HTTP Error: code=401
    Check Register Central Log Contains  Continuing registration with default registration token: ThisIsARegToken

Registering Where Deployment API Check Fails with 500 Falls Back To Default Registration Token
    Register With Local Cloud Server  customer_token=ThisIsACustomerToken  product_selection=fail_500
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken
    Check Register Central Log Contains  Deployment API call failed: MCS HTTP Error: code=500
    Check Register Central Log Contains  Continuing registration with default registration token: ThisIsARegToken

*** Keywords ***
Backup Version Ini
    Copy File  ${SOPHOS_INSTALL}/base/VERSION.ini  /tmp/VERSION.ini.bk

Restore Version Ini
    Move File  /tmp/VERSION.ini.bk  ${SOPHOS_INSTALL}/base/VERSION.ini