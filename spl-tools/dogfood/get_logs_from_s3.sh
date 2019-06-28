#!/bin/bash
set -x
set -e

[[ -n $DEST ]] || DEST=/mnt/uk-filer6/linux/SSPLDogfood/

aws s3 mv s3://sspl-dogfood-feedback/ "$DEST" --recursive --region eu-west-1

# Copy logs from S3 into dated and versioned folders on filer6
#python "${0%/*}/processFeedback.py" $INCOMING $DEST

# Update Dogfood LogMonitor UI
#python "${0%/*}/logMonitor.py" "${0%/*}/cloud_machines.txt" "${0%/*}/opm_machines.txt"

# Remove logs older than 4 weeks
#find $DEST -mindepth 1 -path "${DEST}OLD LOGS" -prune -o -type d -ctime +28 -print -exec rm -rf {} \;
