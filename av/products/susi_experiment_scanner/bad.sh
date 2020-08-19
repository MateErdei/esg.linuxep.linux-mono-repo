#!/bin/bash
STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
cd "${STARTINGDIR}"

export GOOD=bad

exec "$BASE/runScanner2.sh" git-cmd.exe
