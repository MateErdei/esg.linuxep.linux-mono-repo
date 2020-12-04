#!/bin/bash
#
# Copyright 2020, Sophos Limited.  All rights reserved.
#

BASE_DIR=$1
[ -n "$BASE_DIR" ] || BASE_DIR=/tmp_test/encoded_eicars
mkdir -p "${BASE_DIR}"
cd "${BASE_DIR}"

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

# UTF-8
japanese="JAPANESE-ソフォスレイヤーアクセスる"
chinese="CHINESE-涴跺最唗郔場腔醴腔"
korean="KOREAN-디렉토리입다"
french="FRENCH-à ta santé âge"
english="ENGLISH-For all good men"

encodings=( EUC-JP "UTF-8" SJIS LATIN1 )

for file in "$japanese" "$chinese" "$korean" "$french" "$english" ; do
    for encoding in "${encodings[@]}" ; do
        encoding_name=${encoding%=*}
        encoding_iconv=${encoding#*=}

        fullfile="$encoding_name-${file}-VIRUS"
        if [[ $encoding_iconv == UTF-8 ]]
        then
            path=$fullfile
        else
            path=`echo $fullfile | iconv -f UTF-8 -t $encoding_iconv -s -c 2>/dev/null`
        fi
        if [ $? -eq 0 ] ; then
            echo "${path}"
            echoEicar >"${path}"
            [ -f "${path}" ] || echo "Failed to write to ${path}"
        else
            echo Failed to encode to $encoding_name
        fi
    done
done

XXD=`which xxd 2>/dev/null`
if [ -x "$XXD" ] ; then
    path=`echo "3f96d1385bda886d8015019d020709321013202233445566778899aabbccddeeff09e1b9a97beaf97802fffe36e81db2c6de654d23f6" | xxd -p -r -c 256`
    fullpath="RANDOMGARBAGE-${path}-VIRUS"
    echo "${fullpath}"
    mkdir -p `dirname "${fullpath}"`
    echoEicar > "${fullpath}"

    ## Nasty directory
    path=`echo "2e2f2e0d2f5749455244504154482d65696361722e636f6d2d56495255530a" | xxd -r -ps`
    echo "${path}"
    mkdir -p `dirname "${path}"`
    echoEicar > "${path}"

    ## Full ASCII
    FULLASCII="0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e"
    FULLASCII="${FULLASCII}1f202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c"
    FULLASCII="${FULLASCII}3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a"
    FULLASCII="${FULLASCII}5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778"
    FULLASCII="${FULLASCII}797a7b7c7d7e7f"

    path=`echo "$FULLASCII" | xxd -r -ps`
    path="FULL-ASCII-$path"
    echo "${path}"
    mkdir -p "$(dirname "${path}")"
    echoEicar > "${path}"

    ## ASCII without slash
    ASCII="0102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e"
    ASCII="${ASCII}1f202122232425262728292a2b2c2d2e303132333435363738393a3b3c"
    ASCII="${ASCII}3d3e3f404142434445464748494a4b4c4d4e4f505152535455565758595a"
    ASCII="${ASCII}5b5c5d5e5f606162636465666768696a6b6c6d6e6f707172737475767778"
    ASCII="${ASCII}797a7b7c7d7e7f"

    path=`echo "$ASCII" | xxd -r -ps`
    path="ASCII-$path"
    echo "${path}"
    echoEicar > "${path}"

    ## Español in both NFC and NFD
    ## http://www.ericsink.com/entries/quirky.html
    ESNFC="45737061c3b16f6c"
    path=`echo "$ESNFC" | xxd -r -ps`
    path="ES-NFC-$path"
    echo "${path}"
    echoEicar > "${path}"
    path=`echo "$ESNFC" | xxd -r -ps`
    path="ES-$path"
    echo "${path}"
    echoEicar > "${path}"

    ESNFD="457370616ecc836f6c"
    path=`echo "$ESNFD" | xxd -r -ps`
    path="ES-NFD-$path"
    echo "${path}"
    echoEicar > "${path}"
    path=`echo "$ESNFD" | xxd -r -ps`
    path="ES-$path"
    echo "${path}"
    echoEicar > "${path}"

    ## .. and unicode char 200C (zero-width non-joiner) between them - in UTF-8
    JOINER="2ee2808c2e"
    path=`echo "$JOINER" | xxd -r -ps`
    echo "${path}"
    echoEicar > "${path}"

    ## unicode char 200C (zero-width non-joiner) in UTF-8
    JOINER="e2808c"
    path=`echo "$JOINER" | xxd -r -ps`
    echo "${path}"
    echoEicar > "${path}"

    ## ..<200c>/..<200c>/bin/bash
    JOINER="2e2ee2808c2f2e2ee2808c"
    path=$(echo "$JOINER" | xxd -r -ps)
    mkdir -p "${path}/bin"
    path="$path/bin/bash"
    echo "${path}"
    echoEicar > "${path}"

    ## Joiner followed by one dot.
    JOINER="e2808c2e"
    path=`echo "$JOINER" | xxd -r -ps`
    mkdir -p "${path}"
    path="$path/eicar.com"
    echo "${path}"
    echoEicar > "${path}"

    ## dot followed by joiner - looks precisely like current dir hard link
    JOINER="2ee2808c"
    path=`echo "$JOINER" | xxd -r -ps`
    mkdir -p "${path}"
    path="$path/eicar.com"
    echo "${path}"
    echoEicar > "${path}"


else
    echo "*** No xxd" >&2
fi

## HTML
path="<bold>BOLD<font size=+5>THIS IS SOME BIGGER TEXT"
fullpath="HTML-${path}-VIRUS"
echo "${fullpath}"
mkdir -p `dirname "${fullpath}"`
echoEicar > "${fullpath}"

## Quotes
path="SingleDoubleQuote-\"-VIRUS.com"
echoEicar > "${path}"
path="PairDoubleQuote-\"VIRUS.com\""
echoEicar > "${path}"
path="SingleSingleQuote-'-VIRUS.com"
echoEicar > "${path}"
path="PairSingleQuote-'VIRUS.com'"
echoEicar > "${path}"

## White space / rootkit-ish
path=" "
echoEicar > "${path}"
path="
"
echoEicar > "${path}"
path="..."
echoEicar > "${path}"
path="NEWLINEDIR
/
/bin"
mkdir -p "${path}"
path="${path}/sh"
echoEicar > "${path}"


## Windows restrictions
mkdir -p Windows
echoEicar >"Windows/cantendwith."
echoEicar >"Windows/ \\[]:;|=.*?. "
echoEicar >"Windows/CON"
echoEicar >"Windows/AUX"
echoEicar >"Windows/COM1"
echoEicar >"Windows/COM2"
echoEicar >"Windows/COM3"
echoEicar >"Windows/COM4"
echoEicar >"Windows/LPT1"
echoEicar >"Windows/LPT2"
echoEicar >"Windows/LPT3"
echoEicar >"Windows/PRN"
echoEicar >"Windows/NUL"

## Normal name
path="eicar.com"
echoEicar >"${path}"
