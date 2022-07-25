#!/bin/bash

# Copyright 2019, Sophos Limited.  All rights reserved.

# This script is intended to be used to gather diagnose logs from our dogfood machines and save them to S3

function failure()
{
    echo "FAILURE: $@"
    exit 1
}

## get diagnose file
INST=/opt/sophos-spl
$INST/bin/sophos_diagnose /tmp/ || failure "Failed to run diagnose"
FILENAME=$(ls -t /tmp/sspl-diagnose* | head -1)
echo "Uploading: $FILENAME"
[[ -f "$FILENAME" ]] || failure "Failed to create file"
BASENAME=$(basename $FILENAME) || failure "Failed to get basename from full path"

## S3 credentials
S3_USER_NAME="SSPLDogfood"
S3_ACCESS_KEY_ID="AKIAWR523TF7Y4IAJCIX"
S3_SECRET_ACCESS_KEY="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"


S3_BUCKET=sspl-dogfood-feedback

if [[ -x $(which curl) ]]
then
    [[ -x $(which openssl) ]] || failure "Please install openssl"

    key_id="$S3_ACCESS_KEY_ID"
    key_secret="$S3_SECRET_ACCESS_KEY"
    path="$HOSTNAME/$BASENAME"
    bucket="$S3_BUCKET"
    content_type="application/octet-stream"
    date="$(LC_ALL=C date -u +"%a, %d %b %Y %X %z")"
    md5="$(openssl md5 -binary < "$FILENAME" | base64)"

    sig="$(printf "PUT\n$md5\n$content_type\n$date\n/$bucket/$path" | openssl sha1 -binary -hmac "$key_secret" | base64)"

    curl -v -T $FILENAME https://$bucket.s3.amazonaws.com/$path \
        -H "Date: $date" \
        -H "Authorization: AWS $key_id:$sig" \
        -H "Content-Type: $content_type" \
        -H "Content-MD5: $md5" || failure "Failed to upload to bucket: $?"
else
    failure "We don't have anything to upload the diagnose archive with: Please install curl"
fi

rm -f $FILENAME
