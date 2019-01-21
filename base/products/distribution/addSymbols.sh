#!/usr/bin/env bash
[[ -n $SOPHOS_INSTALL ]] || SOPHOS_INSTALL="$1"
[[ -n $SOPHOS_INSTALL ]] || SOPHOS_INSTALL="/opt/sophos-spl"

STARTINGDIR="$(pwd)"
SCRIPTDIR="${0%/*}"
if [[ "$SCRIPTDIR" == "$0" ]]
then
    SCRIPTDIR="${STARTINGDIR}"
fi

ABS_SCRIPTDIR=$(cd $SCRIPTDIR && pwd)

cd $SOPHOS_INSTALL
cp -av $ABS_SCRIPTDIR/files/* .

for file in $(find . -name *.debug)
do
    (
        cd $(dirname $file)
        base=$(basename $file)
        target=${base%.debug}
        echo $base $target
        objcopy --add-gnu-debuglink=$base $target
    )
done
