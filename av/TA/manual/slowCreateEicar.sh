#!/bin/bash
EICAR=
EICAR='H*'$EICAR
EICAR='$H+'$EICAR
EICAR='ST-FILE!'$EICAR
EICAR='RUS-TE'$EICAR
EICAR='IVI'$EICAR
EICAR='DARD-ANT'$EICAR
EICAR='TAN'$EICAR
EICAR='CAR-S'$EICAR
EICAR='CC)7}$EI'$EICAR
EICAR='X54(P^)7'$EICAR
EICAR='@AP[4\PZ'$EICAR
EICAR='!P%'$EICAR
EICAR='5O'$EICAR
EICAR='X'$EICAR

echoEicar()
{
     echo "$EICAR"
     return 0
}

for F in /tmp/altdir/eicar.com /tmp/altdir/subdir/eicar.com /tmp/dir/eicar.com \
    /tmp/dir/subdir/eicar.com
do
    mkdir -p "${F%/*}"
done

sleep 2

echo PID=$$
ls -l /proc/$$/exe

for F in /tmp/altdir/eicar.com /tmp/altdir/subdir/eicar.com /tmp/dir/eicar.com \
    /tmp/dir/subdir/eicar.com /tmp/eicar.com /tmp/eicar.com.com \
    /tmp/eicar.exe /tmp/noteicar.com
do
    echoEicar >$F
done

sleep 1

