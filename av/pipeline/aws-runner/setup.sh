#!/bin/bash
SRC=/home/pair/mav
BUILD=${BUILD:-/home/pair/mav/output}
DEST=.
SUPPLEMENT=/opt/test/inputs

set -ex

rsync -va $SRC/pipeline/aws-runner/ $DEST/aws_runner
rsync -va $SRC/TA/ $DEST/test_scripts
rsync -va $BUILD/ $DEST/av
python3 $SRC/TA/manual/downloadSupplements.py $DEST
