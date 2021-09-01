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

Successful Register With Cloud And Migrate To Another Cloud Server
    [Tags]  AMAZON_LINUX  MCS  MCS_ROUTER  FAKE_CLOUD
    Override LogConf File as Global Level  DEBUG
    Start System Watchdog
    Register With Local Cloud Server
    Check Correct MCS Password And ID For Local Cloud Saved
    Check Cloud Server Log Does Not Contain  Processing deployment api call
    Check Cloud Server Log Contains  Register with ::ThisIsARegToken
    File Exists With Permissions  ${SOPHOS_INSTALL}/logs/base/register_central.log  root  root  -rw-------

    Trigger Migration Now

    Log To Console  Sleeping
    Sleep   300

    # Create Migration Action
    # Migrate via sending MCS action
    # Check Migration



*** Keywords ***
Backup Version Ini
    Copy File  ${SOPHOS_INSTALL}/base/VERSION.ini  /tmp/VERSION.ini.bk

Restore Version Ini
    Move File  /tmp/VERSION.ini.bk  ${SOPHOS_INSTALL}/base/VERSION.ini