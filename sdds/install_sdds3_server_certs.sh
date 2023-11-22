#!/bin/bash

set -ex

cd "${0%/*}" || exit

ROOT_CERT=../common/TA/utils/server_certs/server-root.crt

if [[ ! -f "$ROOT_CERT" ]]
then
    echo "Can't find ROOT_CERT: $ROOT_CERT"
    exit 1
fi

if [[ -d /usr/local/share/ca-certificates ]]
then
    if ! diff -qN "${ROOT_CERT}" /usr/local/share/ca-certificates/root-ca.crt
    then
        sudo cp "${ROOT_CERT}" /usr/local/share/ca-certificates/root-ca.crt
        sudo update-ca-certificates
    else
        echo "${ROOT_CERT}" /usr/local/share/ca-certificates/root-ca.crt already match!
    fi
elif [[ -d /etc/pki/ca-trust/source/anchors/ ]]
then
    if ! diff -qN "${ROOT_CERT}" /etc/pki/ca-trust/source/anchors/root-ca.crt
    then
        sudo cp "${ROOT_CERT}" /etc/pki/ca-trust/source/anchors/root-ca.crt
        sudo update-ca-trust extract
    fi
fi
