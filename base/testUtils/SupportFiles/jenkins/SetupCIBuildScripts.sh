#!/bin/bash
set -x
function failure()
{
    echo "$@"
    exit 1
}

function try_command_with_backoff()
{
    for i in {1..5};
    do
      "$@"
      if [[ $? != 0 ]]
      then
        EXIT_CODE=1
        sleep 15
      else
        EXIT_CODE=0
        break
      fi
    done
    return $EXIT_CODE
}

SCRIPTDIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )
PYTHONCOMMAND=python3
if [[ -d /home/jenkins ]]
then
  PYTHON38="$(which python3.8)"
  [[ -x "${PYTHON38}" ]] && PYTHONCOMMAND=python3.8
fi
try_command_with_backoff  $PYTHONCOMMAND -m pip install --upgrade pip
try_command_with_backoff  $PYTHONCOMMAND -m pip install --ignore-installed PyYAML
PIP_ARGS="-i https://artifactory.sophos-ops.com/artifactory/api/pypi/pypi/simple --trusted-host artifactory.sophos-ops.com"
try_command_with_backoff  $PYTHONCOMMAND -m pip install --upgrade pip ${PIP_ARGS}
try_command_with_backoff  $PYTHONCOMMAND -m pip install wheel ${PIP_ARGS}
try_command_with_backoff  $PYTHONCOMMAND -m pip install --upgrade tap ${PIP_ARGS}  || failure "Unable to install tap"
try_command_with_backoff  $PYTHONCOMMAND -m pip install --upgrade keyrings.alt ${PIP_ARGS}  || failure "Unable to install dependency"
try_command_with_backoff  $PYTHONCOMMAND -m pip install --upgrade artifactory ${PIP_ARGS}  || failure "Unable to install dependency"

if [[ $1 == "--download-pip-cache" ]]
then
  rm -rf pipCache
  mkdir pipCache
  try_command_with_backoff $PYTHONCOMMAND -m pip download tap keyrings.alt signing==2.7.7 --dest pipCache ${PIP_ARGS}  || failure "Unable to install tap"
  tar cvzf $2/pipCache.tar.gz pipCache
  rm -rf pipCache
fi

#Create temporary location used by scripts
sudo mkdir -p /SophosPackages
sudo chown jenkins:jenkins /SophosPackages
sudo chmod 777 /SophosPackages
