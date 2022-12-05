BASE_DIR=$1
[ -n "$BASE_DIR" ] || BASE_DIR=/tmp_test/bad_unicode_eicars
mkdir -p "${BASE_DIR}"

cd "${BASE_DIR}" || {
    echo "Can't go to $BASE_DIR"
    exit 1
}

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
}

echoEicar > $'FFFE8-\xEF\xBF\xBE'

echoEicar > $'FFFF8-\xEF\xBF\xBF'

echoEicar > $'\xEF\xBF\xBE\xEF\xBF\xBF'

