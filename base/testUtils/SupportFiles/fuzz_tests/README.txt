How to run mcsrouter fuzz test so far
--------------------------------------

1. To run fuzz test script you must be root
    # ./vagrant/everrest-systemtest/Supportfiles/fuzz_tests/mcs_fuzz_test_runner.py

    To run on other machines (non-vagrant)
        -> pip install kittyfuzzer, katnip & psutil first
        -> # python mcs_fuzz_test_runner.py
        -> http://localhost:26000/ will give you a visual of the payload and reports during the test run
2. All operational logging:
    i. ./tmp/cloudServer.log
    ii. ./kittylogs/kitty_timestamp.logs test execution summary and failed tests
    iii. ./kittylogs 'last_served_policies_test_<test number>_timestamp.json' contains the last 3 policies before a failure was detected

3. Test results will be summarised as below at the end of the run
    2019-07-15 16:28:39,608 INFO fuzz-test-runner:
                            --------------------------------------------------
                            Finished fuzzing session
                            Target: McsRouterTarget

                            Tested 10 mutations
                            Failure count: 2
                            --------------------------------------------------

    Logs for a failing test:
     2019-07-15 16:28:38,576 - fuzz-test-runner - ERROR - MCS Router Log at "/opt/sophos-spl/logs/base/sophosspl/mcsrouter.log" does contain: Caught exception at top-level
     2019-07-15 16:28:38,576 ERROR fuzz-test-runner: MCS Router Log at "/opt/sophos-spl/logs/base/sophosspl/mcsrouter.log" does contain: Caught exception at top-level
     2019-07-15 16:28:38,579 - fuzz-test-runner - WARNING - Test 9 status: failed
     2019-07-15 16:28:38,579 WARNING fuzz-test-runner: Test 9 status: failed

4. Test script check for any mcsrouter process restarts/crashes and Critcal error logs,
    ->Check cloudServer.log & mcsrouter.log for any other interesting logs.
    -> mcsrouter must not stop, or throw exceptions. this is checked by is_victim_alive() in MCSController
    -> mcsrouter logs must not contain errors
    -> other applications consuming mcsrouter output must run smooth, no crashing, clean logs "Check not implemented"
    -> *** each xmlelemt within the policy is fuzzed as per flags and the policy is logged in cloudServer.log.

5. Extend the coverage by defining new templates of data that is to be fuzzed, see details in Other details



Other details:
---------------
For the kittyfuzzer framework and the complimentary katnip packages.
    b. kittyfuzzer  -> https://kitty.readthedocs.io/en/latest/index.html
    a. katnip       -> https://katnip.readthedocs.io/en/stable/katnip.html (legos package)

    Defining further templates - test_xml_creation.py is included to help in building and testing templates before applying them
        a.  Sepcific to mcs an XmlPolicy can be represented as below and upon reconstruction will have individual components fuzzed as per the flags passed
             mcs_inner_elements = [
                   XmlElement(name='meta', element_name='meta', attributes=[
                       XmlAttribute(name='protoVer', attribute='protocolVersion', value="1.1")
                   ], fuzz_name=False/True, fuzz_content=True/False, delimiter='\n'),

               ]
               mcs_policy_attr = [
                   XmlAttribute(name='xmlns', attribute='xmlns:csc', value='com.sophos\msys\csc', fuzz_attribute=False/True, fuzz_value=True/False),
               ]

               mcs_policy_fuzzed = XmlElement(name='policy', element_name='policy', attributes=mcs_policy_attr,
                                              content=mcs_inner_elements, fuzz_name=False/True, fuzz_content=True/False, delimiter='\n')


