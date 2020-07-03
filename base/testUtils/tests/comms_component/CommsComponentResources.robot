*** Keywords ***

Check Watchdog Starts Comms Component
    Wait Until Keyword Succeeds
    ...  5s
    ...  1s
    ...  Check Comms Component Running

    Wait Until Keyword Succeeds
    ...  10s
    ...  2s
    ...  Check Comms Component Log Contains  Logger CommsComponent configured for level: INFO

    Check Watchdog Log Contains  Starting /opt/sophos-spl/base/bin/CommsComponent