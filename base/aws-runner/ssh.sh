#!/bin/bash

SCRIPT_DIR="${0%/*}"
cd $SCRIPT_DIR

HOST=$1
shift
if [[ "$HOST" != *@* ]]
then
    USERNAME=${USERNAME:-${2:-ec2-user}}
    shift
    HOST="$USERNAME@$HOST"
fi

exec ssh \
    $SSH_ARGS \
    -i $SCRIPT_DIR/ssh-keys/regressiontesting.pem \
    "$HOST" "$@"

