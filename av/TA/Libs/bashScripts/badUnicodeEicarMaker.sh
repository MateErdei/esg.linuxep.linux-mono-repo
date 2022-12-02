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
     return 0
}

path=`echo "EFBFBE" | xxd -r -ps`
path="FFFE8-${path}"
echo "${path}"
echoEicar > "${path}"

path=`echo "EFBFBF" | xxd -r -ps`
path="FFFF8-${path}"
echo "${path}"
echoEicar > "${path}"

path=`echo "EFBFBEEFBFBF" | xxd -r -ps`
path="FFFEFFFF8-${path}"
echo "${path}"
echoEicar > "${path}"

