#!/bin/bash
set -x
set -e

[[ -n $DEST ]] || DEST=/mnt/uk-filer6/linux/SSPL/SSPLDogfood/

aws s3 mv s3://sspl-dogfood-feedback/ "$DEST" --recursive --region eu-west-1
