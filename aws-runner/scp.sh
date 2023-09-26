#!/bin/bash

SCRIPT_DIR="${0%/*}"
cd $SCRIPT_DIR

exec scp \
    -i $SCRIPT_DIR/ssh-keys/regressiontesting.pem \
    "$@"
