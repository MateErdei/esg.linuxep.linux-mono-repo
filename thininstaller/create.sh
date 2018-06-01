#!/bin/bash

TOKEN=$1
URL=$2
CREDS=$3
OUTPUT=$4

cat installer_header.sh >$OUTPUT
echo >>$OUTPUT
echo "TOKEN=$TOKEN" >>$OUTPUT
echo "URL=$URL" >>$OUTPUT
echo "UPDATE_CREDENTIALS=$CREDS" >>$OUTPUT
echo "__ARCHIVE_BELOW__" >>$OUTPUT
cat installer.tar.gz >>$OUTPUT

chmod 700 $OUTPUT
