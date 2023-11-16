# Everest - SystemProductTests

This repository contains all product and system tests for the Everest Project. 
Product and System tests are created using the Python-based Robot Framework: http://robotframework.org/

### Running Tests

The robot tests require that your dev machine has been set up (https://sophos.atlassian.net/wiki/spaces/LD/pages/39042169567/How+To+Build+Base+and+setup+a+dev+machine) and that the esg.linuxep.sspl-tools/setup_vagrant_wsl.sh script has been run. For this readme we'll change directory into the testUtils dir.
```sh
$ cd testUtils
```
```sh
$ ./robot -t <Test Name> .
```
For example:
```sh
$ ./robot -t "Verify that the full installer works correctly" .
```

This will generate a log file on your WSL machine in the current dir. You can view the log by either running Explorer.exe from WSL and the log will open in your default browser, or you can navigate there by hand from Windows explorer (click linux on the left or use this path \\wsl.localhost\Ubuntu\home).
```sh
$ Explorer.exe log.html
```

You can also run suites or pass in any other robot arguments to the ./robot wrapper:

```sh
# Run all tests
$ ./robot .

# Run a directory of suites
$ ./robot --suite mcs_router .

# Run a single suite within a directory
$ ./robot --suite TestCommandPoll .

# Run a single test
$ ./robot --test CommandPollSent .

# Exclude tests by tag by running the following
$ ./robot --exclude manual .
```

It is also possible to run the tests in tap. 

```
tap run sspl_base.integration --debug-loop
```

Running system tests with custom inputs
1. Build the component you are updating in CI

2. On CI, build a branch for your changes under sspl-warehouse (e.g. develop/orange , develop/pear,  develop/abelard, etc.)
    a. alter the inputs in dev.xml to point to the component you have built

3. Alter system-product-test-release-package.xml to point to the warehouse you have built

The following tags can be used to select which tests can be run, using the include or exclude arguments.
* AMAZON_LINUX - Test cases which will run on AWS only
* AV_PLUGIN - Test cases which test anti-virus plugin
* BASE_DOWNGRADE - Test cases where base is downgraded
* CENTRAL - Tests which run against Central or Nova
* DATAFEED - Datafeed test cases
* DEBUG - Tests which are useful for debugging
* DIAGNOSE - Tests which exercise sophos_diagnose
* EDR_PLUGIN - Tests focusing on EDR capabilities
* EVENT_JOURNALER_PLUGIN - Tests which exercise the event journaler plugin
* EXCLUDE_AWS - Used to exclude the test from being run on AWS
* EXCLUDE_RHEL8  excludes the test from running on rhel 8
* FAKE_CLOUD - Tests which run against our fake central scripts
* FAULTINJECTION - Tests that deliberately introduces faults to the normal installation
* FUZZ - Tests that are related to fuzzer
* INSTALLER - Tests which exercise the main installer 
* LOAD${N} - Tags used to roughly balance the load of test jobs which can run in parallel
* LIVERESPONSE_PLUGIN - Tests which exercise the live response plugin
* LOGGING - Tests which exercise the products logging
* MANAGEMENT_AGENT - Tests which exercise the Management Agent component
* MANUAL - Tests which are ony run manually and not as part of a test run on Jenkins
* MCS - Tests which use the MCS pipeline
* MCS_ROUTER - Tests which exercise the MCS Router component
* MESSAGE_RELAY - Tests which include a Message Relay
* PLUGIN_DOWNGRADE - Test cases where a plugin is downgraded
* OSTIA - Tests which use ostia
* RESPONSE_ACTIONS_PLUGIN - Tests which exercise the ra plugin
* REGISTRATION - Tests that exercise the registration code
* RUNTIMEDETECTIONS_PLUGIN - Tests focusing on RunTimedetections capabilities
* SAV - Tests which install SAV
* SLOW - Tests which take a long time to run
* SMOKE - Tests which check that the most important functions of the product work
* SULDOWNLOADER - Tests that exercise SUL Downloader
* SYSTEMPRODUCTTESTINPUT - Tests that fail on TAP as they require this.
* TAP_PARALLEL{N} - Tags used to balance TAP test load.
* TELEMETRY - Tests that exercise the Telemetry executable
* TESTFAILURE - Tests that we expect to fail
* THIN_INSTALLER - Tests that exercise the Thin Installer
* UNINSTALL - Tests that exercise the Uninstaller code
* UPDATE_CACHE - Tests that use an Update Cache
* UPDATE_SCHEDULER - Tests that exercise the Update Scheduler plugin
* WATCHDOG - Tests that exercise the Watchdog
* WAREHOUSE_SYNC - Test that fail if the warehouse and base build are out of sync
* WDCTL - Tests that exercise WDCTL, Watchdog Control