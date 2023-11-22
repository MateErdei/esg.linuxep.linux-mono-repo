#!/bin/bash

cd "${0%/*}/../.." || exit
if [ ! -f SophosSetup.sh ]
then
  echo "Place the thin installer with your credentials (e.g. downloaded from Central) as SophosSetup.sh in the root of the repo"
  exit 1
fi
mv SophosSetup.sh SophosSetup.sh.tmp
tar -xf .output/thininstaller/thininstaller/*.tar.gz
rm -rf manifest.dat
./thininstaller/test/extractAndCombineThinInstallers.sh SophosSetup.sh.tmp SophosSetup.sh
rm -f SophosSetup.sh.tmp SophosSetup.sh
mv SophosSetupFinal.sh SophosSetup.sh
