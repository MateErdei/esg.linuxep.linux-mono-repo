#!/bin/bash
set -xe

#
# Script to run an arbitrary script on a remote machine
#


function failure()
{
    echo "$@" >&2
    exit 1
}

(( "$#" >= "2" )) || failure "Usage: $0 <config_script> <user@target_machine> [options...]"

config_script="$1"
shift
target_machine="$1"
shift
options="$@"

if [[ -n "$ZERO_FREE_SPACE" ]]
then
    env_vars="ZERO_FREE_SPACE=$ZERO_FREE_SPACE"
fi

this_dir_path=$(readlink -f $(dirname $0))
[[ -e "${this_dir_path}/${config_script}" ]] || failure "Failed to find ${this_dir_path}/${config_script}"

this_dir_name=$(basename ${this_dir_path})
echo rsync -e "ssh -o StrictHostKeyChecking=no" -av ${this_dir_path}/ ${target_machine}:${this_dir_name}/
rsync -e "ssh -o StrictHostKeyChecking=no" -av ${this_dir_path}/ ${target_machine}:${this_dir_name}/
if [[ "$?" != "0" ]]
then
    scp -o StrictHostKeyChecking=no -r ${this_dir_path}/ ${target_machine}:${this_dir_name}/
fi

ssh -o StrictHostKeyChecking=no -t ${target_machine} "$env_vars ./${this_dir_name}/${config_script} ${options}"