#!/usr/bin/env bash

set -x

function pip_install()
{
  local python_target="$1"
  python3 -c "import ${python_target}" 2> /dev/null
  if [[ "$?" != "0" ]];then
    sudo -E python3 -m pip install ${python_target}
  else
    echo "Python package ${python_target} already installed"
  fi
}

python3 -m pip install pip --upgrade

# install dependencies
for python_package in sseclient aiohttp aiohttp_sse asyncio python-dateutil websockets packaging protobuf==3.14.0
do
  pip_install ${python_package}
done
