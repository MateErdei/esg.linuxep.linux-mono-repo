#!/bin/bash

for i in $@
do
    case $i in
        --noproxy)
            no_proxy=true
        ;;
    esac
    shift
done

## set https_proxy if required
if [[ -z $no_proxy ]] && [[ -n $https_proxy ]]
then
    export http_proxy="http://allegro.eng.sophos:3128/"
    export https_proxy="$http_proxy"
fi

function failure()
{
    echo "FAILURE: $@"
    exit 1
}

## get diagnose file
INST=/opt/sophos-spl
$INST/bin/sophos_diagnose /tmp/ || failure "Failed to run diagnose"
FILENAME=$(ls -t /tmp/sspl-diagnose* | head -1)

[[ -f "$FILENAME" ]] || failure "Failed to create file"
BASENAME=$(basename $FILENAME) || failure "Failed to get basename from full path"

## S3 credentials
S3_USER_NAME="DogFoodFeedback"
S3_ACCESS_KEY_ID="AKIAJD6FZV5XHAI5FNXQ"
S3_SECRET_ACCESS_KEY="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"

S3_BUCKET=sspl-dogfood-feedback

if [[ -x $(which curl) ]]
then
    [[ -x $(which openssl) ]] || failure "Please install openssl"

    key_id="$S3_ACCESS_KEY_ID"
    key_secret="$S3_SECRET_ACCESS_KEY"
    path="$BASENAME"
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
