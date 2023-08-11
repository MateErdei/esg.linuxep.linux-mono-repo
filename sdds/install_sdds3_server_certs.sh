#!/bin/bash

set -ex

cd "${0%/*}" || exit

HTTPS_DIR=TA/SupportFiles/https

[[ -d $HTTPS_DIR ]] || {
    echo "Can't find HTTPS_DIR: $HTTPS_DIR"
    exit 1
}

make -C "${HTTPS_DIR}"

if [[ -d /usr/local/share/ca-certificates ]]
then
    if ! diff -qN "${HTTPS_DIR}/root-ca.crt.pem" /usr/local/share/ca-certificates/root-ca.crt
    then
        sudo cp "${HTTPS_DIR}/root-ca.crt.pem" /usr/local/share/ca-certificates/root-ca.crt
        sudo update-ca-certificates
    else
        echo "${HTTPS_DIR}/root-ca.crt.pem" /usr/local/share/ca-certificates/root-ca.crt already match!
    fi
elif [[ -d /etc/pki/ca-trust/source/anchors/ ]]
then
    if ! diff -qN "${HTTPS_DIR}/root-ca.crt.pem" /etc/pki/ca-trust/source/anchors/root-ca.crt
    then
        sudo cp "${HTTPS_DIR}/root-ca.crt.pem" /etc/pki/ca-trust/source/anchors/root-ca.crt
        sudo update-ca-trust extract
    fi
fi
chmod -R a+rX "${HTTPS_DIR}"