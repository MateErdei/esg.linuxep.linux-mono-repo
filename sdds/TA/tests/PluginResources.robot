*** Settings ***
Library    Collections
Library    OperatingSystem
Library    Process
Library    String

Library    ${COMMON_TEST_LIBS}/LogUtils.py
Library    ${COMMON_TEST_LIBS}/MCSRouter.py
Library    ${COMMON_TEST_LIBS}/OSUtils.py
Library    ${COMMON_TEST_LIBS}/SafeStoreUtils.py

Resource    ${COMMON_TEST_ROBOT}/GeneralUtilsResources.robot


*** Variables ***
${rtdBin}     ${RTD_DIR}/bin/runtimedetections


*** Keywords ***
Runtime Detections Plugin Is Running
    ${result} =    Run Process  pgrep  -f  ${rtdBin}
    Should Be Equal As Integers    ${result.rc}    0    RuntimeDetections Plugin not running

Check RuntimeDetections Installed Correctly
    Wait For RuntimeDetections to be Installed
    Verify RTD Component Permissions
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