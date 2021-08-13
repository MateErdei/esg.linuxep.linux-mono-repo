#!/bin/sh
DEST=${1:-/tmp_test/three_hundred_eicars}
COUNT=${2:-300}


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
     echo $EICAR
     return 0
}


mkdir -p $DEST
for x in $(seq 1 1 ${COUNT})
do
        echoEicar > $DEST/eicar$x
done
