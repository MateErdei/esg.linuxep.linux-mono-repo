#!/bin/bash

function die()
{
    echo "$@" >&2
    exit 1
}

(( "$#" >= "2" )) || die "Usage: $0 <config_filename> <user@target_machine> [options...]"

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
[[ -e "${this_dir_path}/${config_script}" ]] || die "Failed to find ${this_dir_path}/${config_script}"

this_dir_name=$(basename ${this_dir_path})
rsync -e "ssh -o StrictHostKeyChecking=no" -av ${this_dir_path}/ ${target_machine}:${this_dir_name}/
if [[ "$?" != "0" ]]
then
    scp -o StrictHostKeyChecking=no -r ${this_dir_path}/ ${target_machine}:${this_dir_name}/
fi
set -x
ls -l ./${this_dir_name}/${config_script}
ssh -o StrictHostKeyChecking=no -t ${target_machine} "$env_vars ./${this_dir_name}/${config_script} ${options}; rm -rf ./${this_dir_name}"
