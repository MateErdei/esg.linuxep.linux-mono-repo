A set of tools for running SSPL test passes in AWS.

To run the system product tests against a specific build use the following command

```bash
./run_tests_in_aws.sh <PathToBaseDist> <PathToExampleDist> <PathToThinInstallerDist> <PathToEventProcessor> <PathToAuditPlugin> <PathToEDRPlugin> <PathToMDRPlugin> <PathToMDRComponentSuite> <PathToSystemProductTestOutputTarGzFile> <PathToLiveresponsePlugin>
```
for e.g
```bash
./run_tests_in_aws.sh /uk-filer5/prodro/bir/sspl-base/0-5-0-87/213185/output/SDDS-COMPONENT/ /uk-filer5/prodro/bir/sspl-exampleplugin/0-5-0-22/213187/output/SDDS-COMPONENT /uk-filer5/prodro/bir/sspl-thininstaller/0-5-0-0-10/213201/output /uk-filer5/prodro/bir/sspl-base/0-5-0-87/213185/output/SystemProductTestOutput.tar.gz /uk-filer5/prodro/bir/liveterminal_linux/1-1-6/220232/
```
OR

To run against the latest SSPL local jenkins builds on filer6 (filer6/linux/SSPL/JenkinsBuildOutput/)
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

Tips for running tests manually

Comment out the the line bash /opt/sspl/testAndSendResults.sh in the User data section of platform you are running in the sspl-system.template file

use test.sh to run without shutting down the machine and sending results afterwards. This script takes regular robot arguments:
e.g. -t <testname> -i <tag> -s <suite>

To change which version of sspl-base testcode you are running
For running tests locally you will need to change the SYSTEM_TEST_BRANCH in gather.sh
For running with jenkins you need to change the everest-base branch name in the Jenkins file

To run non-master branch in jenkins
For running non master branches in jenkins you will also need to change the sspl-aws-branch name in the JenkinsFile
