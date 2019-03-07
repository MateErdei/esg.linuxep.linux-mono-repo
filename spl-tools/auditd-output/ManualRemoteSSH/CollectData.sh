#!/bin/bash

function RunTest()
{
    ./ClearLogs.sh
    echo Run
    echo ./Local-${1}.sh ${2}
    read -p "Have you run the script" yn
    if [[ ${yn} == "" ]]
    then
        echo "Gathering Logs"
        ./GatherLogs.sh Remote_SSH_${1}
    fi
}

./InitialiseSystem.sh
./CreateTestUser.sh
RunTest failed_ssh_single_attempt_with_key ${1}
RunTest failed_ssh_multiple_attempts_with_key ${1}  
RunTest failed_ssh_command_single_attempt_with_key ${1}
RunTest failed_ssh_command_multiple_attempt_with_key ${1}

if [[ -z ${2} ]] 
then
    echo "Running non-amazon tests"
    ./SetupSSHKey.sh
    RunTest success_ssh_command_multiple_attempt_with_key ${1}
    RunTest success_ssh_command_single_attempt_with_key ${1}
    ./CleanUpSSHKey.sh
else
    echo "Running amazon tests"
    ./SetupAmazonSSHKey.sh
    RunTest success_command_single_attempt_with_key_amazon ${1}
    ./CleanUpAmazonSSHKey.sh
fi

RunTest failed_ssh_multi_password_attempt ${1}
RunTest failed_ssh_password_attempt ${1}
RunTest failed_ssh_password_command_attempt ${1}
RunTest successful_ssh_multi_password_command_attempt ${1}
RunTest successful_ssh_password_command_attempt ${1}

./RemoveTestUser.sh
./Teardown.sh

