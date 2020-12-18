*** Settings ***

Library     ../libs/Cleanup.py

*** Keywords ***

Start Fake Cloud
    Log  Starting Fake Cloud
    register cleanup  Stop Fake Cloud

Stop Fake Cloud
    Log  Stop Fake Cloud
