#!/bin/bash

PLUGINS_DIR="/opt/sophos-spl/plugins"
[[ -d "$PLUGINS_DIR" ]] || exit 1
HOSTNAME=$(hostname)
DATETIME=$(date -u +"%Y/%m/%d %H:%M:%S")


for plugin_dir in "$PLUGINS_DIR"/*
do
    if [ -d "$plugin_dir" ]
    then
      FILE_COUNT=$(find $plugin_dir -name "*.sst" | wc -l)
      SIZE=$(find $plugin_dir -type f -name "*.sst" -exec du -cb {} + | grep total | sed 's/\s.*$//')
      PLUGIN=$(basename $plugin_dir)

      curl -X POST "http://sspl-perf-mon:9200/osquery-db-files/_doc" -H "Content-Type: application/json" -d \
      '{"hostname":"'$HOSTNAME'", "filecount":'$FILE_COUNT', "filesize":'$SIZE', "datetime":"'"$DATETIME"'", "plugin":"'$PLUGIN'"}'
    fi
done

