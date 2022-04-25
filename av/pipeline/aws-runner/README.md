Runs SSPL-AV on platforms not supported by TAP
==============================================

Steps required:
1. Package input:
   1.1. TA/packageArtifactoryBuild.sh esg-build-tested/linuxep.sspl-plugin-anti-virus/develop/20220424040000-48427cd7b45f4b52da67fb131f51bec04c62e6c6-52BHvV/
2. bunzip2 -c /tmp/inputs.tar.bz2 | gzip - >/tmp/inputs.tar.gz
3. export TEST_TAR=/tmp/inputs.tar.gz
4. export SKIP_GATHER=1
5. export SSHLocation=80.229.30.66/32
6. pipeline/aws-runner/run_tests_in_aws.sh



Base instructions
=================

A set of tools for running SSPL test passes in AWS.

The test inputs for this script are defined by the <everest-base>/testUtils/system-product-test-release-package.xml

Arguments:
robot framework arguments (DONT USE WHITESPACE IN TEST NAMES, USE UNDERSCORES) are gathered from the "aws-runner/robotArgs" file.
each line of args will run on a seperate set of AMIs.
e.g.:

```
cat <<EOT > ./argFile
-i LOAD1
-i LOAD2
-e LOAD1 -e LOAD2
EOT
```

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


Tips for running tests manually:

Comment out the the line bash /opt/sspl/testAndSendResults.sh (and below) in the User data section of platform you are
running in the sspl-system.template file so that the machine is not turned off before you can ssh onto it

use test.sh to run without shutting down the machine and sending results afterwards. This script takes regular robot arguments:
e.g. -t <testname> -i <tag> -s <suite>
