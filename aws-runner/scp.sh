#!/bin/bash

SCRIPT_DIR="${0%/*}"
cd $SCRIPT_DIR

TOOLS_DIR=$SCRIPT_DIR/../../../savlinux/tools

exec scp \
    -i $SCRIPT_DIR/regressiontesting.pem \
    -i $TOOLS_DIR/amazon-private-key-ukdevsavlinux.pem \
    -i $TOOLS_DIR/amazon-private-key-ukdevsavlinux2.pem \
    "$@"
