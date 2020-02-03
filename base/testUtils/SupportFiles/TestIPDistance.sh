#!/usr/bin/env bash

PROXY_UTILS=${0%/*}/../libs/ProxyUtils.py
[[ -f $PROXY_UTILS ]] || { echo "Failed to find $PROXY_UTILS" ; exit 1 ; }


IPADDR="10.101.102.43"

IFS='.' read -ra ADDR <<< "$IPADDR"

DIST_0="${ADDR[0]}.${ADDR[1]}.${ADDR[2]}.$(( ADDR[3] ))"     #Distance 0
DIST_1="${ADDR[0]}.${ADDR[1]}.${ADDR[2]}.$(( ADDR[3] ^ 1 ))" #Distance 1
DIST_2="${ADDR[0]}.${ADDR[1]}.${ADDR[2]}.$(( ADDR[3] ^ 2 ))" #Distance 2
DIST_3="${ADDR[0]}.${ADDR[1]}.${ADDR[2]}.$(( ADDR[3] ^ 4 ))" #Distance 3
DIST_4="${ADDR[0]}.${ADDR[1]}.${ADDR[2]}.$(( ADDR[3] ^ 8 ))"
DIST_5="${ADDR[0]}.${ADDR[1]}.${ADDR[2]}.$(( ADDR[3] ^ 16 ))" #Distance 5
DIST_6="${ADDR[0]}.${ADDR[1]}.${ADDR[2]}.$(( ADDR[3] ^ 32 ))"
DIST_7="${ADDR[0]}.${ADDR[1]}.${ADDR[2]}.$(( ADDR[3] ^ 64 ))"
DIST_8="${ADDR[0]}.${ADDR[1]}.${ADDR[2]}.$(( ADDR[3] ^ 128 ))"
DIST_9="${ADDR[0]}.${ADDR[1]}.$(( ADDR[2] ^ 1)).${ADDR[3]}" #Distance 9
DIST_16="${ADDR[0]}.${ADDR[1]}.$(( ADDR[2] ^ 128)).${ADDR[3]}"
DIST_17="${ADDR[0]}.$(( ADDR[1] ^ 1 )).${ADDR[2]}.${ADDR[3]}"
DIST_24="${ADDR[0]}.$(( ADDR[1] ^ 128 )).${ADDR[2]}.${ADDR[3]}"
DIST_25="$(( ADDR[0] ^ 1 )).${ADDR[1]}.${ADDR[2]}.${ADDR[3]}" #Distance 25
DIST_26="$(( ADDR[0] ^ 2 )).${ADDR[1]}.${ADDR[2]}.${ADDR[3]}"
DIST_27="$(( ADDR[0] ^ 4 )).${ADDR[1]}.${ADDR[2]}.${ADDR[3]}"
DIST_28="$(( ADDR[0] ^ 8 )).${ADDR[1]}.${ADDR[2]}.${ADDR[3]}"
DIST_29="$(( ADDR[0] ^ 16 )).${ADDR[1]}.${ADDR[2]}.${ADDR[3]}"
DIST_30="$(( ADDR[0] ^ 32 )).${ADDR[1]}.${ADDR[2]}.${ADDR[3]}"
DIST_31="$(( ADDR[0] ^ 64 )).${ADDR[1]}.${ADDR[2]}.${ADDR[3]}"
DIST_32="$(( ADDR[0] ^ 128 )).${ADDR[1]}.${ADDR[2]}.${ADDR[3]}"

python ${PROXY_UTILS} 0 ${DIST_0}
python ${PROXY_UTILS} 1 ${DIST_1}
python ${PROXY_UTILS} 2 ${DIST_2}
python ${PROXY_UTILS} 3 ${DIST_3}
python ${PROXY_UTILS} 4 ${DIST_4}
python ${PROXY_UTILS} 5 ${DIST_5}
python ${PROXY_UTILS} 6 ${DIST_6}
python ${PROXY_UTILS} 7 ${DIST_7}
python ${PROXY_UTILS} 8 ${DIST_8}
python ${PROXY_UTILS} 9 ${DIST_9}
python ${PROXY_UTILS} 16 ${DIST_16}
python ${PROXY_UTILS} 17 ${DIST_17}
python ${PROXY_UTILS} 24 ${DIST_24}
python ${PROXY_UTILS} 25 ${DIST_25}
python ${PROXY_UTILS} 26 ${DIST_26}
python ${PROXY_UTILS} 27 ${DIST_27}
python ${PROXY_UTILS} 28 ${DIST_28}
python ${PROXY_UTILS} 29 ${DIST_29}
python ${PROXY_UTILS} 30 ${DIST_30}
python ${PROXY_UTILS} 31 ${DIST_31}
python ${PROXY_UTILS} 32 ${DIST_32}
