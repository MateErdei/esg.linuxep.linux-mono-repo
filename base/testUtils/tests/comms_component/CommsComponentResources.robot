*** Keywords ***

Check Watchdog Starts Comms Component
    Wait Until Keyword Succeeds
    ...  5s
    ...  1s
    ...  Check Comms Component Is Running

    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Comms Component Log Contains  Logger comms_component configured for level: INFO


    Check Watchdog Log Contains  Starting /opt/sophos-spl/base/bin/CommsComponent

Verify All Mounts Have Been Removed
    [Arguments]   ${jailPath}
    Check Not A MountPoint  ${jailPath}/etc/resolv.conf
    Check Not A MountPoint  ${jailPath}/usr/lib64
    Check Not A MountPoint  ${jailPath}/etc/hosts
    Check Not A MountPoint  ${jailPath}/usr/lib
    Check Not A MountPoint  ${jailPath}/lib
    Check Not A MountPoint  ${jailPath}/etc/ssl/certs
    Check Not A MountPoint  ${jailPath}/etc/pki/tls/certs
    Check Not A MountPoint  ${jailPath}/base/mcs/certs


Cleanup mounts
    [Arguments]   ${jailPath}
    Run Process  umount  ${jailPath}/etc/hosts
    Run Process  umount  ${jailPath}/etc/resolv.conf
    Run Process  umount  ${jailPath}/usr/lib64
    Run Process  umount  ${jailPath}/lib
    Run Process  umount  ${jailPath}/usr/lib
    Run Process  umount  ${jailPath}/etc/ssl/certs
    Run Process  umount  ${jailPath}/etc/pki/tls/certs
    Run Process  umount  ${jailPath}/base/mcs/certs/
    Run Process  umount  ${jailPath}/etc/pki/ca-trust/extracted

Check Not A MountPoint
    [Arguments]  ${mount}
    ${res} =  Run Process  findmnt  -M  ${mount}
    Should Not Be Equal As Integers   ${res.rc}  0