*** Settings ***
Library    Collections
Library    OperatingSystem
Library    Process
Library    String

Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/ProcessUtils.py
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/OSUtils.py
Library    ${COMMON_TEST_LIBS}/SafeStoreUtils.py

Resource    ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot


*** Variables ***
# RTD
${rtdBin}     ${RTD_DIR}/bin/runtimedetections
${RUNTIME_DETECTIONS_LOG_PATH}  ${RTD_DIR}/log/runtimedetections.log

*** Keywords ***
Runtime Detections Plugin Is Running
    ${result} =    Run Process  pgrep  -f  ${rtdBin}
    Should Be Equal As Integers    ${result.rc}    0    RuntimeDetections Plugin not running

Check RuntimeDetections Installed Correctly
    Wait For RuntimeDetections to be Installed
    Verify RTD Component Permissions
    Wait For Rtd Log Contains After Last Restart    ${RUNTIME_DETECTIONS_LOG_PATH}    Analytics started processing telemetry   timeout=${30}
    Verify Running RTD Component Permissions

Wait For RuntimeDetections to be Installed
    Wait Until Created    ${rtdBin}    timeout=40s
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   1 secs
    ...   Runtime Detections Plugin Is Running

Verify RTD Component Permissions
    ${result} =   Run Process    find ${RTD_DIR} | xargs ls -ld    shell=True   stderr=STDOUT
    Log   ${result.stdout}

    Verify Permissions    ${RTD_DIR}  0o750    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/bin                      0o710    root   sophos-spl-group
    Verify Permissions    ${rtdBin}                           0o710    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/bin/uninstall.sh         0o710    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/etc                             0o750    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/etc/sensor.yaml                 0o640    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/etc/analytics.yaml              0o640    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/etc/content_rules/rules.yaml    0o640    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/log                          0o700    sophos-spl-user   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/log/runtimedetections.log    0o600    sophos-spl-user   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/var  0o750    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/var/lib                        0o750    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/var/lib/struct_layouts.json    0o640    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/var/lib/bpf                0o750    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/var/lib/bpf/dummy.bpf.o    0o640    root   sophos-spl-group
    Verify Permissions    ${RTD_DIR}/var/lib/bpf/dummy.bpf.c    0o640    root   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/var/run    0o750    sophos-spl-user   sophos-spl-group

    Verify Permissions    ${RTD_DIR}/VERSION.ini    0o640    root   sophos-spl-group

Verify Running RTD Component Permissions
    Verify Permissions    ${RTD_DIR}/var/run/cache_analytics.yaml    0o640    sophos-spl-user   sophos-spl-group

    Verify Permissions    /var/run/sophos   0o700   sophos-spl-user   sophos-spl-group
    Verify Permissions    /run/sophos       0o700   sophos-spl-user   sophos-spl-group

Wait Until RTD Cgroup Check Completes
    Wait Until Keyword Succeeds
    ...   40 secs
    ...   1 secs
    ...   Verify Running RTD Cgroup Permissions

Verify Running RTD Cgroup Permissions
    IF   ${{os.path.isdir("/sys/fs/cgroup/")}}
        IF   ${{os.path.isdir("/sys/fs/cgroup/systemd/runtimedetections/")}}
            Verify Permissions   /sys/fs/cgroup/systemd/runtimedetections   0o755    sophos-spl-user   sophos-spl-group
        ELSE IF   ${{os.path.isdir("/sys/fs/cgroup/runtimedetections/")}}
            Verify Permissions   /sys/fs/cgroup/runtimedetections   0o755    sophos-spl-user   sophos-spl-group
        ELSE
            Fail   Cannot find cgroup directory for runtimedetections
        END
    END

Verify AV Processes Are Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/av/sbin/av
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/av/sbin/safestore
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/av/sbin/soapd
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/av/sbin/sophos_threat_detector

Wait For AV To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For Av Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify AV Processes Are Running    ${install_path}

Verify Base Processes Are Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/base/bin/sophos_management
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/base/bin/python3
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/base/bin/UpdateScheduler
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/base/bin/tscheduler
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/base/bin/sophos_watchdog

Wait For Base To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For Base Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify Base Processes Are Running    ${install_path}

Verify DI Process Is Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    # TODO: once DI is in current shipping, remove extra check
    ${binary_file_exists} =    Check File Exists    ${install_path}/plugins/deviceisolation/bin/deviceisolation
    IF    ${binary_file_exists}
        Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/deviceisolation/bin/deviceisolation
    END

Wait For DI To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For Deviceisolation Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify DI Process Is Running    ${install_path}

Verify EDR Processes Are Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/edr/bin/edr
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/edr/bin/osqueryd

Wait For EDR To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For EDR Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify EDR Processes Are Running    ${install_path}

Verify EJ Process Is Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/eventjournaler/bin/eventjournaler

Wait For EJ To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For EJ Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify EJ Process Is Running    ${install_path}

Verify LR Process Is Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/liveresponse/bin/liveresponse

Wait For LR To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For Liveresponse Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify LR Process Is Running    ${install_path}

Verify RTD Process Is Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/runtimedetections/bin/runtimedetections

Wait For RTD To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For RTD Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify RTD Process Is Running    ${install_path}

Verify RA Process Is Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Wait Until Keyword Succeeds    30s    2s    Pidof Or Fail    ${install_path}/plugins/responseactions/bin/responseactions

Wait For RA To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For Response Action Logs To Indicate Plugin Is Ready    log_marks=${log_marks}
    Verify RA Process Is Running    ${install_path}

Check Plugin Processes Are Running
    [Arguments]    ${install_path}=${SOPHOS_INSTALL}
    Verify AV Processes Are Running    ${install_path}
    Verify Base Processes Are Running    ${install_path}
    Verify DI Process Is Running    ${install_path}
    Verify EDR Processes Are Running    ${install_path}
    Verify EJ Process Is Running    ${install_path}
    Verify LR Process Is Running    ${install_path}
    Verify RTD Process Is Running    ${install_path}
    Verify RA Process Is Running    ${install_path}

Wait For Plugins To Be Ready
    [Arguments]    ${log_marks}    ${install_path}=${SOPHOS_INSTALL}
    Wait For Plugins Logs To Indicate Plugins Are Ready    log_marks=${log_marks}
    Check Plugin Processes Are Running    ${install_path}