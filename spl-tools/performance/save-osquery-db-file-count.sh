#!/bin/bash

HOSTNAME=$(hostname)
PLUGINS_DIR="/opt/sophos-spl/plugins"
FILE_COUNT=$(find $PLUGINS_DIR -name "*.sst" | wc -l)
SIZE=$(find $PLUGINS_DIR -type f -name *.sst -exec du -cb {} + | grep total | sed 's/\s.*$//')
DATETIME=$(date +"%Y/%m/%d %H:%M:%S")

curl -X POST "http://sspl-perf-mon:9200/osquery-db-files/_doc" -H "Content-Type: application/json" -d '{"hostname":"'$HOSTNAME'", "filecount":'$FILE_COUNT', "filesize":'$SIZE', "datetime":"'"$DATETIME"'"}'
