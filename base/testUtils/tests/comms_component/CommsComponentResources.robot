*** Keywords ***

Check Watchdog Starts Comms Component
    Wait Until Keyword Succeeds
    ...  5s
    ...  1s
    ...  Check Comms Component Running

    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Comms Component Log Contains  Logger comms_component configured for level: INFO


    Check Watchdog Log Contains  Starting /opt/sophos-spl/base/bin/CommsComponent

Verify All Mounts Have Been Removed
    [Arguments]   ${jailPath}=${JAIL_PATH}
    Check Not A MountPoint  ${jailPath}/etc/resolv.conf
    Check Not A MountPoint  ${jailPath}/usr/lib64
    Check Not A MountPoint  ${jailPath}/etc/hosts
    Check Not A MountPoint  ${jailPath}/usr/lib
    Check Not A MountPoint  ${jailPath}/lib
    Check Not A MountPoint  ${jailPath}/etc/ssl/certs
    Check Not A MountPoint  ${jailPath}/etc/pki/tls/certs
    Check Not A MountPoint  ${jailPath}/base/mcs/certs

Check Not A MountPoint
    [Arguments]  ${mount}
    ${res} =  Run Process  findmnt  -M  ${mount}
    Should Not Be Equal As Integers   ${res.rc}  0