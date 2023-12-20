#!/bin/bash
set -e
# A crude script to save a bit of time when running local tests by working out what tap args to pass in.
# Manual example:
# TAP_PARAMETER_ROBOT_TEST="On access gets IDE update to new scanners" ./spl-tools/tests/qemu_do.py tap run linux_mono_repo.products.testing.system_tests.TAP_PARALLEL2_linux_x64_rel_ubuntu2004_x64_aws_server_en_us
# This wrapper example:
# ./tap_test.sh -t "On access gets IDE update to new scanners"

cd "${0%/*}"
DRY_RUN="0"
while [[ $# -ge 1 ]]
do
    case $1 in
          --test|-t)
            shift
            export TAP_PARAMETER_ROBOT_TEST="$1"
            ;;
          --dry-run|-d)
            DRY_RUN="1"
            ;;
        # TODO We can add suites later if people find this useful.
          *)
            echo "Unknown option"
            exit 2
            ;;
    esac
    shift
done

ubuntu_x64="linux_x64_rel_ubuntu2004_x64_aws_server_en_us"
if [[ -n "$TAP_PARAMETER_ROBOT_TEST" ]]
then
    # Currently this will match the first test only, so beware if you're running a test
    # that has the same name as another. This could be improved later if people use this script.
    test_file=$(grep -riIl --include \*.robot "$TAP_PARAMETER_ROBOT_TEST" | head -1)
    echo "File: \"$test_file\""
    parallel_job=$(grep -h -o -m 1 -E "TAP_PARALLEL[0-9]"  "$test_file" || echo -n "")
    echo "Job (if applicable): \"$parallel_job\""

    if [[ "$test_file" == *"sdds/"* ]]
    then
        echo "System test"
        stage="linux_mono_repo.products.testing.system_tests.${parallel_job}_${ubuntu_x64}"
    elif [[ "$test_file" == *"av/"* ]]
    then
        echo "AV component test"
        stage="linux_mono_repo.products.testing.av_tests.integration_${parallel_job}_${ubuntu_x64}"
    elif [[ "$test_file" == *"base/"* ]]
    then
        echo "Base component test"
        stage="linux_mono_repo.products.testing.base_tests.${parallel_job}_${ubuntu_x64}"
    elif [[ "$test_file" == *"edr/"* ]]
    then
        echo "EDR component test"
        stage="linux_mono_repo.products.testing.edr_tests.integration_${parallel_job}_${ubuntu_x64}"
    elif [[ "$test_file" == *"deviceisolation/"* ]]
    then
        echo "Device Isolation component test"
        stage="linux_mono_repo.products.testing.deviceisolation_tests.${ubuntu_x64}"
    elif [[ "$test_file" == *"liveterminal/"* ]]
    then
        echo "LiveTerminal component test"
        stage="linux_mono_repo.products.testing.liveterminal_tests.component_${ubuntu_x64}"
    elif [[ "$test_file" == *"eventjournaler/"* ]]
    then
        echo "Event Journaler component test"
        stage="linux_mono_repo.products.testing.eventjournaler_tests.${ubuntu_x64}"
    else
        echo "Couldn't work out where the test is: $TAP_PARAMETER_ROBOT_TEST"
        exit 1
    fi

    if [[ "$DRY_RUN" == "1" ]]
    then
        # Just print the tap command
        echo TAP_PARAMETER_ROBOT_TEST=\""$TAP_PARAMETER_ROBOT_TEST"\" ./spl-tools/tests/qemu_do.py tap run "${stage}"
    else
        ./spl-tools/tests/qemu_do.py tap run "${stage}"
    fi

fi


