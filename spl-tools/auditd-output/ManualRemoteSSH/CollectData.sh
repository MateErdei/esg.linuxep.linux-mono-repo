#!/bin/bash
RED=`tput setaf 1`
RESET=`tput sgr0`
function CheckExitShell()
{
    read -p "${RED}Have you closed the remote shell on your other terminal?${RESET}"
}

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
    RunTest success_ssh_command_single_attempt_with_key ${1}
    RunTest success_ssh_command_multiple_attempt_with_key ${1}
    RunTest success_ssh_no_command_single_attempt_with_key ${1}
    CheckExitShell
    ./CleanUpSSHKey.sh
else
    echo "Running amazon tests"
    ./SetupAmazonSSHKey.sh
    ./SetupAWSsshd_config.sh
    RunTest success_ssh_command_single_attempt_with_key_amazon ${1}
    RunTest success_ssh_command_multiple_attempts_with_key_amazon ${1}
    RunTest success_ssh_no_command_single_attempt_with_key_amazon ${1}
    CheckExitShell
    ./CleanUpAmazonSSHKey.sh
fi

RunTest failed_ssh_password_attempt ${1}
RunTest failed_ssh_multi_password_attempt ${1}
RunTest failed_ssh_password_command_attempt ${1}
RunTest successful_ssh_multi_password_command_attempt ${1}
RunTest successful_ssh_password_command_attempt ${1}
RunTest successful_ssh_password_no_command_attempt ${1}
CheckExitShell

./RemoveTestUser.sh
./Teardown.sh

