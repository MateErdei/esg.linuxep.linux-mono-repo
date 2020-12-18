*** Settings ***

Library     ../libs/Cleanup.py
Library     ../libs/LocalCloud.py

*** Keywords ***
Start Fake Cloud
    Log  Starting Fake Cloud
    LocalCloud.start_local_fake_cloud
    register cleanup  Stop Fake Cloud

Stop Fake Cloud
    Log  Stop Fake Cloud
    LocalCloud.stop_local_fake_cloud
