#!/bin/bash
STARTINGDIR=$(pwd)
cd ${0%/*}
BASE=$(pwd)
cd "${STARTINGDIR}"

export GOOD=good

exec "$BASE/runScanner2.sh" git-cmd.exe
