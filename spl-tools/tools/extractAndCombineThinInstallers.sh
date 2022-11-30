#!/bin/bash

if [ $# -ne 2 ]; then
echo "Usage:"
echo "   ./extractAndCombineInstallers.sh <InstallerWithCreds> <InstallerWithoutCreds>"
echo "---Will create the installer called SophosSetupFinal.sh from the two given installers"
exit
fi

sed -n "1,/^__MIDDLE_BIT__/p" $2 > SophosSetup.sh
sed -n '/^__MIDDLE_BIT__/,/^__ARCHIVE_BELOW__/p' $1 | head -n-1 | tail -n+2 >> SophosSetup.sh
sed -n '/^__ARCHIVE_BELOW__/,$p' $2 >>SophosSetup.sh
chmod +x SophosSetup.sh
