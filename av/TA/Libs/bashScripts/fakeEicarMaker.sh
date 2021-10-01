#!/bin/sh
DEST=${1:-/tmp_test/three_hundred_eicars}
COUNT=${2:-300}


EICAR=
EICAR='I'$EICAR
EICAR='amH*'$EICAR
EICAR='not$H+'$EICAR
EICAR='anST-FILE!'$EICAR
EICAR='eicarRUS-TE'$EICAR
EICAR='butIVI'$EICAR
EICAR='IDARD-ANT'$EICAR
EICAR='mightTAN'$EICAR
EICAR='beCAR-S'$EICAR
EICAR='youCC)7}$EI'$EICAR
EICAR='willX54(P^)7'$EICAR
EICAR='never@AP[4\PZ'$EICAR
EICAR='know!P%'$EICAR
EICAR='for5O'$EICAR
EICAR='realX'$EICAR


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
