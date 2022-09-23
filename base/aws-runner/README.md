A set of tools for running SSPL test passes in AWS.

The test inputs for this script are defined by the <everest-base>/testUtils/system-product-test-release-package.xml

## Arguments:
robot framework arguments (DONT USE WHITESPACE IN TEST NAMES, USE UNDERSCORES) are gathered from the "aws-runner/robotArgs" file.
each line of args will run on a seperate set of AMIs.
e.g.:

"""
cat <<EOT > ./argFile
-i LOAD1
-i LOAD2
-e LOAD1 -e LOAD2
EOT
"""

will run 3 sets of templates running "-i LOAD1", "-i LOAD2", and "-e LOAD1 -e LOAD2" respectively

```bash
./run_tests_in_aws.sh
```

To run subset of platforms
To run only rhel platforms
RUNSOME=rhel ./run_tests_in_aws.sh

To run only amazon platforms
RUNSOME=amazonlinux ./run_tests_in_aws.sh

To run only ubuntu platforms
RUNSOME=ubuntu ./run_tests_in_aws.sh

To run a single platform
RUNONE=<hostname> ./run_tests_in_aws.sh


-----


## Tips for running tests manually:
Remove the following lines from the `UserData` section of the platform.json you are running so that the machine is not turned off before you can ssh onto it:
- `"bash /opt/sspl/testAndSendResults.sh ", {"Ref": "StackName"}, " @ARGSGOHERE@ >> /tmp/cloudFormationInit.log 2>&1\n",`
- `"echo 'Finished.' >> /tmp/cloudFormationInit.log\n"`
- `"aws s3 cp /tmp/cloudFormationInit.log s3://sspl-testbucket/test-results/", { "Ref": "StackName" }, "/$HOSTNAME-cloudFormationInit.log\n",`
- `"poweroff\n"`

Run `run_tests_in_aws.sh` with the following arguments:
1. STACK - To prevent tarring up the test inputs for each run you can specify this so TEST_TAR doesn't change
2. SKIP_GATHER - Once the inputs have been tarred up once, this will skip the gather and simply copy over the local copy
3. Arg to determine which platform(s) - RUNSOME, RUNONE

Once on the machine:
1. `tail -f /tmp/cloudFormationInit.log` to ensure the machine is setup correctly
2. `cd /opt/sspl`
3. Now you can run `test.sh` to run without shutting down the machine and sending results afterwards. This script takes regular robot arguments: e.g. -t <testname> -i <tag> -s <suite>

If you are unable to connect to the AWS machine via SSH replace the `NetworkInterfaces` section of the json with:
`"SecurityGroupIds": ["sg-09bb5dbd7eb4ec5fb"],`
