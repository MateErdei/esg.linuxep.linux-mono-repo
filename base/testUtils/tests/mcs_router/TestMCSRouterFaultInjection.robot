*** Settings ***
Library  OperatingSystem
Library    ${LIBS_DIRECTORY}/FilesystemWatcher.py
Library    ${LIBS_DIRECTORY}/TemporaryDirectoryManager.py
Library    ${LIBS_DIRECTORY}/OSUtils.py

Resource  McsRouterResources.robot
Suite Setup  Suite Setup
Test Setup  Test Setup
Test Teardown  Test Teardown

#Test Teardown  Require Uninstalled

Default Tags  MCS  FAKE_CLOUD  MCS_ROUTER

*** Test Cases ***

changed of actions folder to block sophos-spl user from writing
    Check Cloud Server Log For Command Poll
    Chmod  550  /opt/sophos-spl/base/mcs/action

    Trigger Update Now
    # Action will not be received until the next command poll
    Check Cloud Server Log For Command Poll    2
    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSrouter Log Contains    Failed to write an action to: /opt/sophos-spl/tmp/actions
    Check Action In Atomic Files


Overwriting action file
    Chmod  550  /opt/sophos-spl/base/mcs/action

    Trigger Update Now
    # Action will not be received until the next command poll
    Check Cloud Server Log For Command Poll    2
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check MCSrouter Log Contains    Failed to write an action to: /opt/sophos-spl/tmp/actions
    ${filepath} =  Check Action In Atomic Files
    ${contents} =  Get File  ${filepath}

    Create File   ${filepath}    content=fakecontents
    Chmod  750  /opt/sophos-spl/base/mcs/action
    Wait Until Keyword Succeeds
    ...  10 secs
    ...  2 secs
    ...  Check MCSrouter Log Contains    Distribute new action: ALC_action_FakeTime.xml

    Wait Until Keyword Succeeds
    ...  5 secs
    ...  1 secs
    ...  Check Managementagent Log Contains  Action /opt/sophos-spl/base/mcs/action/ALC_action_FakeTime.xml sent to 1 plugins
    Check UpdateScheduler Log Contains  Unexpected action xml received: fakecontents

Repeatedly writing the same file into the action folder Doesnt Cause A Crash

    sleep  2
    Stop MCSRouter

    Remove File  /opt/sophos-spl/logs/base/suldownloader.log

    ${temp_dir} =  add_temporary_directory  staging
    Create File   ${temp_dir}/template    content=<?xml version='1.0'?><action type="sophos.mgt.action.ALCForceUpdate"/>
    Chmod  550  ${temp_dir}/template
    Chown  sophos-spl-user:sophos-spl-group  ${temp_dir}/template

    Check UpdateScheduler Log Contains String N Times  Attempting to update from warehouse  1

    Copy File And Send It To MCS Actions folder  ${temp_dir}/template
    Copy File And Send It To MCS Actions folder  ${temp_dir}/template
    Copy File And Send It To MCS Actions folder  ${temp_dir}/template
    Copy File And Send It To MCS Actions folder  ${temp_dir}/template
    Copy File And Send It To MCS Actions folder  ${temp_dir}/template
    Copy File And Send It To MCS Actions folder  ${temp_dir}/template
    Copy File And Send It To MCS Actions folder  ${temp_dir}/template
    Copy File And Send It To MCS Actions folder  ${temp_dir}/template
    Copy File And Send It To MCS Actions folder  ${temp_dir}/template
    Copy File And Send It To MCS Actions folder  ${temp_dir}/template

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check Suldownloader Log Contains  Update failed, with code: 112
    Check Managementagent Log Contains String N Times  Action /opt/sophos-spl/base/mcs/action/ALC_action_FakeTime.xml sent to 1 plugins  10
    Check UpdateScheduler Log Contains String N Times  Attempting to update from warehouse  2
    Check UpdateScheduler Log Contains String N Times  An active instance of SulDownloader is already running, continuing with current instance  9
    Check All Product Logs Do Not Contain Critical


actions is out of space
    [Teardown]  Test Teardown With Mount Removal

    Check Cloud Server Log For Command Poll

    Remove Directory  ${SOPHOS_INSTALL}/tmp/actions
    ${mountpoint} =  Make Tiny File System  5MB  ${SOPHOS_INSTALL}/tmp/actions
    Chmod  700  ${SOPHOS_INSTALL}/tmp/actions
    Chown  sophos-spl-user:sophos-spl-group  ${mountpoint}

    Make File Of Size And Expect No Space Error  ${SOPHOS_INSTALL}/tmp/actions/bigFile  10MB
    Trigger Update Now
    # Action will not be received until the next command poll
    Check Cloud Server Log For Command Poll    2

    Wait Until Keyword Succeeds
    ...  30 secs
    ...  5 secs
    ...  Check MCSrouter Log Contains    utf8 write failed with message: [Errno 28] No space left on device
    Check MCSrouter Log Contains    Failed to write an action to: /opt/sophos-spl/tmp/actions



*** Keywords ***
Make Tiny File System
    [Arguments]  ${size}  ${target_dir}
    ${temp_dir} =  Add Temporary Directory  workspace

    Make File Of Size  ${temp_dir}/fs  5MB
    ${r} =  Run Process  mkfs.ext3  ${temp_dir}/fs
    Should Be Equal As Strings  ${r.rc}  0


    Set Test Variable  ${mountpoint}  ${target_dir}
    Create Directory  ${mountpoint}
    ${r} =  Run Process  mount  ${temp_dir}/fs  ${mountpoint}
    Log  ${r.stderr}
    Log  ${r.stdout}
    Should Be Equal As Strings  ${r.rc}  0
    [Return]  ${mountpoint}

Make File Of Size
    [Arguments]  ${path}  ${size}
    ${r} =  Run Process  dd  if\=/dev/urandom  of\=${path}  bs\=${size}  count\=1
    Log  ${r.stderr}
    Log  ${r.stdout}
    Should Be Equal As Strings  ${r.rc}  0

Make File Of Size And Expect No Space Error
    [Arguments]  ${path}  ${size}
    ${r} =  Run Process  dd  if\=/dev/urandom  of\=${path}  bs\=${size}  count\=1
    Should Contain  ${r.stderr}     dd: error writing '${path}': No space left on device
    Should Exist  ${path}

Cleanup mount
    ${r} =  Run Process  umount  ${mountpoint}
    Should Be Equal As Strings  ${r.rc}  0


Chmod
    [Arguments]  ${settings}  ${path}
    ${r} =  Run Process  chmod  ${settings}  ${path}
    Should Be Equal As Strings  ${r.rc}  0

Chown
    [Arguments]  ${settings}  ${path}
    ${r} =  Run Process  chown  ${settings}  ${path}
    Should Be Equal As Strings  ${r.rc}  0

Stop MCSRouter
    ${r} =  Run Process  /opt/sophos-spl/bin/wdctl  stop  mcsrouter
    Should Be Equal As Strings  ${r.rc}  0
    log  ${r.stdout}
    log  ${r.stderr}
    Remove File  /opt/sophos-spl/logs/base/sophosspl/mcsrouter.log

Copy File And Send It To MCS Actions folder
    [Arguments]  ${file}
    ${directory} =  get_dirname_of_path  ${file}
    ${basename} =  get_basename_of_path  ${file}

    Copy File  ${file}    ${directory}/${basename}-copy
    ${r} =  Run Process  chown  sophos-spl-user:sophos-spl-group  ${directory}/${basename}-copy
    Should Be Equal As Strings  ${r.rc}  0
    Move File  ${directory}/${basename}-copy  /opt/sophos-spl/base/mcs/action/ALC_action_FakeTime.xml

Suite Setup
    Start Local Cloud Server

Test Setup
    Set Local CA Environment Variable
    Require Fresh Install

    Check For Existing MCSRouter
    Cleanup Local Cloud Server Logs

    create file  /opt/sophos-spl/base/mcs/certs/ca_env_override_flag
    Register With Fake Cloud

Test Teardown
    Check MCSRouter Does Not Contain Critical Exceptions
    MCSRouter Test Teardown
    Stop MCSRouter If Running
    #There are two ways of starting proxy servers so both required
    Stop Proxy Servers
    Stop Proxy If Running

    Cleanup Proxy Logs

    Cleanup Local Cloud Server Logs
    Deregister From Central

#    Cleanup MCSRouter Directories
    Remove File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs_policy.config
    Remove File  ${SOPHOS_INSTALL}/base/etc/sophosspl/mcs.config
    # ensure no other msc router is running even those not created by this test.
    Kill Mcsrouter
    Unset_CA_Environment_Variable

Test Teardown With Mount Removal
    Cleanup mount
    Test Teardown