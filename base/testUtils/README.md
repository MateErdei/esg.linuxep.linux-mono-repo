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

The following tags can be used to select which tests can be run, using the include or exclude arguments.
* AUDIT_PLUGIN - Test cases which test sspl-audit
* AMAZON_LINUX - Test cases which will run on Amazon Linux only
* CENTRAL - Tests which run against Central or Nova
* DEBUG - Tests which are useful for debugging
* DIAGNOSE - Tests which exercise sophos_diagnose
* EVENT_PLUGIN - Tests which exercise the event processor plugin 
* EXAMPLE_PLUGIN - Tests which exercise the example plugin 
* EXCLUDE_AWS - Used to exclude the test from being run on AWS
* CUSTOM_LOCATION - Tests which install to a custom location
* FAKE_CLOUD - Tests which run against our fake central scripts
* INSTALLER - Tests which exercise the main installer 
* MANAGEMENT_AGENT - Tests which exercise the Management Agent component
* MANUAL - Tests which are ony run manually and not as part of a test run on Jenkins
* MCS - Tests which use the MCS pipeline
* MCS_ROUTER - Tests which exercise the MCS Router component
* MESSAGE_RELAY - Tests which include a Message Relay
* MDR_PLUGIN - Tests which exercise the mdr plugin
* EDR_PLUGIN - Tests focusing on EDR capabilities
* PUB_SUB - Tests which use the protobuf pub sub pipeline
* REGISTRATION - Tests that exercise the registration code
* OSTIA - Tests which use ostia
* SAV - Tests which install SAV
* SLOW - Tests which take a long time to run
* SMOKE - Tests which check that the most important functions of the product work
* SULDOWNLOADER - Tests that exercise SUL Downloader
* TELEMETRY - Tests that exercise the Telemetry executable
* TESTFAILURE - Tests that we expect to fail
* THIN_INSTALLER - Tests that exercise the Thin Installer
* UNINSTALL - Tests that exercise the Uninstaller code
* UPDATE_CACHE - Tests that use an Update Cache
* UPDATE_SCHEDULER - Tests that exercise the Update Scheduler plugin
* WATCHDOG - Tests that exercise the Watchdog
* WDCTL - Tests that exercise WDCTL, Watchdog Control
* FUZZ - Tests that are related to fuzzer
* FAULTINJECTION - Tests that deliberately introduces faults to the normal installation
* MDR_REGRESSION_TESTS - End to end tests related to MTR feature
* TAP_TESTS - Tests to be executed in Tap environment