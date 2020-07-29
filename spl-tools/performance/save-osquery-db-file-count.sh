#!/bin/bash

PLUGINS_DIR="/opt/sophos-spl/plugins"
[[ -d "$PLUGINS_DIR" ]] || exit 1
HOSTNAME=$(hostname)
DATETIME=$(date -u +"%Y/%m/%d %H:%M:%S")


for plugin_dir in "$PLUGINS_DIR"/*
do
    if [ -d "$plugin_dir" ]
    then
      PLUGIN=$(basename $plugin_dir)
      FILE_COUNT=$(find $plugin_dir -name "*.sst" | wc -l)
      if (( $FILE_COUNT == 0 ))
      then
          echo "Skipping: $PLUGIN"
          continue
      else
        echo "Sending count for: $PLUGIN"
      fi
      SIZE=$(find $plugin_dir -type f -name "*.sst" -exec du -cb {} + | grep total | sed 's/\s.*$//')

      curl -X POST "http://sspl-perf-mon:9200/osquery-db-files/_doc" -H "Content-Type: application/json" -d \
      '{"hostname":"'$HOSTNAME'", "filecount":'$FILE_COUNT', "filesize":'$SIZE', "datetime":"'"$DATETIME"'", "plugin":"'$PLUGIN'"}'
    fi
done

