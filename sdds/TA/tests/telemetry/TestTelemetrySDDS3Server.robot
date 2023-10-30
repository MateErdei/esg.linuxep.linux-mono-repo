
*** Settings ***
Suite Setup      Upgrade Resources Suite Setup
Suite Teardown   Upgrade Resources Suite Teardown

Test Setup       Require Uninstalled
Test Teardown    Run Keywords
...                Remove Environment Variable  http_proxy    AND
...                Remove Environment Variable  https_proxy  AND
...                Stop Proxy If Running    AND
...                Stop Proxy Servers   AND
...                Clean up fake warehouse  AND
...                Upgrade Resources SDDS3 Test Teardown

Resource    ${COMMON_TEST_ROBOT}/SDDS3Resources.robot
Resource    ${COMMON_TEST_ROBOT}/TelemetryResources.robot
Resource    ${COMMON_TEST_ROBOT}/UpgradeResources.robot

Force Tags    TAP_PARALLEL3

*** Test Cases ***
Telemetry Executable Moves All Top Level Telemetry Items ESM Enabled
    # TODO: LINUXDAR-8281: Fix and re-enable local SDDS3 server system tests
    [Tags]  DISABLED
    Cleanup Telemetry Server

    Setup SUS static
    ${fixed_version_token} =    read_token_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${fixed_version_name} =    read_name_from_warehouse_linuxep_json    ${tmpLaunchDarkly}/${staticflagfile}
    ${esm_enabled_alc_policy} =    populate_fixed_version_with_normal_cloud_sub    ${fixed_version_name}    ${fixed_version_token}

    Create File  ${tmpPolicy}   ${esm_enabled_alc_policy}
    Register Cleanup  Remove File  ${tmpPolicy}

    Start Local Cloud Server  --initial-alc-policy  ${tmpPolicy}

    # LINUXDAR-7059: On SUSE the thin installer fails to connect to the first SDDS3 server so workaround for now by running twice
    ${result} =  Run Process    bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${COMMON_TEST_LIBS}/SDDS3server.py --launchdarkly ${tmpLaunchDarkly} --sdds3 ${VUT_WAREHOUSE_ROOT}  shell=true    timeout=10s
    Log  ${result.stdout}
    Log  ${result.stderr}
    ${handle}=  Start Process  bash -x ${SUPPORT_FILES}/jenkins/runCommandFromPythonVenvIfSet.sh python3 ${COMMON_TEST_LIBS}/SDDS3server.py --launchdarkly ${tmpLaunchDarkly} --sdds3 ${VUT_WAREHOUSE_ROOT}  shell=true

    Set Suite Variable    ${GL_handle}    ${handle}

    configure_and_run_SDDS3_thininstaller  0  https://localhost:8080   https://localhost:8080
    Wait Until Keyword Succeeds
    ...   80 secs
    ...   10 secs
    ...   Check Log Contains String At Least N times    ${SULDownloaderLog}    suldownloader_log   Update success  1
    Check Current Release With AV Installed Correctly

    File Should Exist     ${SOPHOS_INSTALL}/base/update/var/package_config.xml
    Prepare To Run Telemetry Executable

    Run Telemetry Executable     ${EXE_CONFIG_FILE}     ${SUCCESS}
    ${telemetryFileContents} =  Get File    ${TELEMETRY_OUTPUT_JSON}
    LOG    ${telemetryFileContents}

    check_update_scheduler_telemetry_json_is_correct  ${telemetryFileContents}  0   True
        ...    install_state=0
        ...    download_state=0
        ...    base_tag=${fixed_version_name}
    check_base_telemetry_json_is_correct  ${telemetryFileContents}

    @{top_level_list} =  Create List   esmName    esmToken    suiteVersion    tenantId    deviceId
    check_top_level_telemetry_items    ${telemetryFileContents}   ${top_level_list}
