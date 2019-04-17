#!/usr/bin/env bash


# Ensure we run script from within sspl-tools directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
pushd "${SCRIPT_DIR}/../" &> /dev/null

function error()
{
	printf '\033[0;31m'
	echo "ERROR: $1"
	printf '\033[0m'
	exit 1
}

function warning()
{
    printf '\033[01;33m'
    echo "Warning: $1"
    printf '\033[0m'
}

function echoProgress()
{
    printf '\033[1;34m'
    echo "[Step $CURRENT_STEP / $TOTAL_STEPS]: $1"
    printf '\033[0m'
    CURRENT_STEP=$(( CURRENT_STEP + 1 ))
}

function cloneSSPLRepos()
{
    [[ -f ${SCRIPT_DIR}/gitRepos.txt ]] || error "Cannot find gitRepos.txt"

	for repo in $(cat ${SCRIPT_DIR}/gitRepos.txt)
	do
        repoName=$(echo $repo | awk '{n=split($0, a, "/"); print a[n]}' | sed -n "s_\([^\.]*\).*_\1_p")
        if [[ -d $repoName ]]
        then
            warning "Repo $repoName already exists!"
            continue
        fi
		git clone $repo || error "Failed to clone $repo with code $?"
	done
}

# Clone all SSPL Repos
echoProgress "Cloning SSPL Repos"
cloneSSPLRepos
