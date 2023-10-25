#!/usr/bin/env bash

STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)

TAP_VENV_DIR=/opt/tapvenv

function failure()
{
    echo "$@"
    exit 1
}

while [[ $# -ge 1 ]]
do
    case $1 in
        --venv|--setup-venv)
            rm -rf "${TAP_VENV_DIR}"
            PYTHON="${PYTHON:-python3.7}"
            [[ -x "$(which $PYTHON)" ]] || PYTHON=python3
            "${PYTHON}" -m venv "${TAP_VENV_DIR}"
            source "${TAP_VENV_DIR}/bin/activate"
            cat <<EOF >"${TAP_VENV_DIR}/pip.conf"
[global]
timeout=60
index-url = https://artifactory.sophos-ops.com/api/pypi/pypi/simple
EOF
            pip install --upgrade pip
            pip install --upgrade wheel build_scripts
            pip install --upgrade tap keyrings.alt
            exit 0
            ;;
        --fetch|--setup)
            [[ -d ${TAP_VENV_DIR} ]] && source ${TAP_VENV_DIR}/bin/activate
            export TAP_PARAMETER_MODE=release
            export TAP_PARAMETER_BUILD_SELECTION=base
            TAP=${TAP:-tap}
            "$TAP" fetch linux_mono_repo.products.bazel.linux_rel
            exit 0
            ;;
          --package-arm-av)
            shift
            exec bash $BASE/av/TA/packageArmBuild.sh "$@"
            ;;
          *)
            echo "Unknown option"
            exit 2
            ;;
    esac
    shift
done