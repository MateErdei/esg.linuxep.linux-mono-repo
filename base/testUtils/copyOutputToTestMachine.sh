#!/bin/bash

DEST=${1:-pair@ubuntu2004sspl.eng.sophos}

rsync -va testUtils/ "${DEST}:/opt/test/inputs/testUtils" --exclude='*.pyc' --exclude __pycache__
rsync -va output/ "${DEST}:/opt/test/inputs/everest-base/output"
