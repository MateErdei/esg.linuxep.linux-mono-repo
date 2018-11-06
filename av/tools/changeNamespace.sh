#!/usr/bin/env bash

NEW_NAMESPACE=$1
[[ -n ${NEW_NAMESPACE} ]] || {
    echo "FAILURE: required argument new namespace"
    exit 2
}
for F in $(grep -lr namespace plugin/)
do
    echo $F
    sed -i -e "s/namespace [^ ;]*/namespace ${NEW_NAMESPACE}/" $F
done
