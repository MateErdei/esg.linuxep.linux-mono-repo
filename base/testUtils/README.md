# Everest - SystemProductTests

This repository contains all product and system tests for the Everest Project. 
Product and System tests are created using the Python-based Robot Framework: http://robotframework.org/

### Running Tests

The tests require the Robot Framework, Pip, and Python to be installed on the system:

```sh
$ sudo apt-get install python-pip
$ pip install robotframework
```

Once the Robot Framework is installed, change into the root directory and run the following:

```sh
# Run all tests
$ robot .

# Run a directory of suites
$ robot --suite mcs_router .

# Run a single suite within a directory
$ robot --suite TestCommandPoll .

# Run a single tests
$ robot --test CommandPollSent .

# Exclude tests by tag by running the following:
$ robot --exclude manual .
```

It is also possible to run the tests in tap. 

```python
tap run sspl_base.integration --debug-loop
```

Building an Ostia Warehouse Using Builds From Your Pairing Station
1.build base
2.build mtr component suite
3. In sspl-tools, run:
    a.Only needs to be done once:
        setup/setupPairingStationOstiaBranches.py
    b. setup/copyLocalBuildOutputToDevWarehouse.py

4. On CI, build the branch for your pairing station under sspl-warehouse (e.g. develop/orange , develop/pear,  develop/abelard, etc.)

5. In system product tests
    a. go to libs/WarehouseUtils.py
    b. find this line "OSTIA_VUT_ADDRESS_BRANCH = os.environ.get(OSTIA_VUT_ADDRESS_BRANCH_OVERRIDE, "master")"
    c. change "master" to "develop-*yourPairingStationName* (e.g. develop-orange, develop-pear, develop-abelard, etc

DONE. Any tests that would previously have used the VUT ostia warehouse (i.e. master) will now use the builds which were on your pairing station.
To refresh, run step 3b then 4

The following tags can be used to select which tests can be run, using the include or exclude arguments.
* AMAZON_LINUX - Test cases which will run on AWS only
* AUDIT_PLUGIN - Test cases which test sspl-audit
* CENTRAL - Tests which run against Central or Nova
* CUSTOM_LOCATION - Tests which install to a custom location
* DEBUG - Tests which are useful for debugging
* DIAGNOSE - Tests which exercise sophos_diagnose
* EDR_PLUGIN - Tests focusing on EDR capabilities
* EVENT_PLUGIN - Tests which exercise the event processor plugin 
* EXAMPLE_PLUGIN - Tests which exercise the example plugin 
* EXCLUDE_AWS - Used to exclude the test from being run on AWS
* EXCLUDE_CENTOS8 excludes the test from running on centos 8
* EXCLUDE_RHEL8  excludes the test from running on rhel 8
* FAKE_CLOUD - Tests which run against our fake central scripts
* FAULTINJECTION - Tests that deliberately introduces faults to the normal installation
* FUZZ - Tests that are related to fuzzer
* INSTALLER - Tests which exercise the main installer 
* LIVERESPONSE_PLUGIN - Tests which exercise the live response plugin
* MANAGEMENT_AGENT - Tests which exercise the Management Agent component
* MANUAL - Tests which are ony run manually and not as part of a test run on Jenkins
* MCS - Tests which use the MCS pipeline
* MCS_ROUTER - Tests which exercise the MCS Router component
* MDR_PLUGIN - Tests which exercise the mdr plugin
* MDR_REGRESSION_TESTS - End to end tests related to MTR feature
* MESSAGE_RELAY - Tests which include a Message Relay
* PUB_SUB - Tests which use the protobuf pub sub pipeline
* OSTIA - Tests which use ostia
* REGISTRATION - Tests that exercise the registration code
* SAV - Tests which install SAV
* SLOW - Tests which take a long time to run
* SMOKE - Tests which check that the most important functions of the product work
* SULDOWNLOADER - Tests that exercise SUL Downloader
* TAP_TESTS - Tests to be executed in Tap environment
* TELEMETRY - Tests that exercise the Telemetry executable
* TESTFAILURE - Tests that we expect to fail
* THIN_INSTALLER - Tests that exercise the Thin Installer
* UNINSTALL - Tests that exercise the Uninstaller code
* UPDATE_CACHE - Tests that use an Update Cache
* UPDATE_SCHEDULER - Tests that exercise the Update Scheduler plugin
* WATCHDOG - Tests that exercise the Watchdog
* WDCTL - Tests that exercise WDCTL, Watchdog Control
