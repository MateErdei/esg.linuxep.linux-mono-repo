#!/bin/bash

set -e

if [[ $(id -u) == 0 ]]
then
    echo "You don't need to run this as root"
    exit 1
fi

BASEDIR=$(dirname "$0")

# Just in case this script ever gets symlinked
BASEDIR=$(readlink -f "$BASEDIR")
cd "$BASEDIR"
source "$BASEDIR/common_vars.sh"

if [[ -z ${ROOT_LEVEL_BUILD_DIR+x} ]]
then
  echo "ROOT_LEVEL_BUILD_DIR is not set"
  exit 1
fi
if [[ ! -d "$ROOT_LEVEL_BUILD_DIR" ]]
then
  echo "$ROOT_LEVEL_BUILD_DIR does not exist"
  exit 1
fi

if [[ -z ${FETCHED_INPUTS_DIR+x} ]]
then
  echo "FETCHED_INPUTS_DIR is not set"
  exit 1
fi
if [[ ! -d "$FETCHED_INPUTS_DIR" ]]
then
  echo "$FETCHED_INPUTS_DIR does not exist"
  exit 1
fi

if [[ -z ${REDIST+x} ]]
then
  echo "REDIST is not set"
  exit 1
fi

[[ -d "$REDIST" ]] || mkdir -p "$REDIST"

function is_older_than()
{
  local path1="$1"
  local path2="$2"
  path1_date=$(stat -c %Z "$path1")
  path2_date=$(stat -c %Z "$path2")
  [[ $path1_date < $path2_date ]]
}

function should_skip_based_on_date()
{
  local archive=$1
  if [[ ! -f $archive ]]
  then
    echo "$archive does not exist"
    exit 1
  fi
  local should_skip_rc=0
  local should_unpack_rc=1
  local archive_basename=$(basename "$archive")
  local marker_file="$REDIST/$archive_basename-marker"
  if [[ -f "$marker_file" ]]
  then
    if is_older_than "$archive" "$marker_file"
      then
        return $should_skip_rc
      fi
  fi
  return $should_unpack_rc
}

function unpack_tars()
{
  shopt -s nullglob
  for tarfile in "$FETCHED_INPUTS_DIR/"*.tar; do
    local archive_basename=$(basename "$tarfile")
    local marker_file="$REDIST/$archive_basename-marker"
    if should_skip_based_on_date "$tarfile"
    then
      echo "Skipping unpack based on date, already up-to-date: $tarfile"
      continue
    else
      # If archive is newer than marker then check if we need to unpack archive based on hash.
      local archive_hash=$(md5sum "$tarfile" | cut -d ' ' -f 1)
      if [[ -f $marker_file ]]
      then
        echo "$tarfile is newer than $marker_file, will check hash."
        if [[ $(cat "$marker_file") == "$archive_hash" ]]
        then
          # touch the marker so that if we're in the situation where marker is older than archive but is the same,
          # (e.g. someone ran TAP fetch and the archives 'change time' got updated but they're still the same) then
          # next time around we skip the hash check because the marker will be newer than the archive due to the touch.
          touch "$marker_file"
          echo "Skipping unpack based on hash, already up-to-date: $tarfile"
          continue
        fi
      fi
      echo "Extracting .tar: $tarfile - $archive_hash"
      # Remove old dir if needed
      local output_dir_name=$(tar -tf "$tarfile" | cut -f1 -d"/" | sort | uniq | head -n 1)
      local output_dir_full_path="$REDIST/$output_dir_name"
      [[ -d "$output_dir_full_path" ]] && rm -rf "$output_dir_full_path"

      # Remove old marker if needed
      rm -f "$marker_file"

      # Unpack
      tar xf "$tarfile" -C "$REDIST" || exitFailure 1 "Not a valid input .tar"

      # Create marker
      echo "$archive_hash" > "$marker_file"
    fi
  done
  shopt -u nullglob
}

function unpack_gzipped_tars()
{
  shopt -s nullglob
  for tarfile in "$FETCHED_INPUTS_DIR/"*.tar.gz; do
    local archive_basename=$(basename "$tarfile")
    local marker_file="$REDIST/$archive_basename-marker"
    if should_skip_based_on_date "$tarfile"
    then
      echo "Skipping unpack based on date, already up-to-date: $tarfile"
      continue
    fi

    # If archive is newer than dir then check if we need to unpack archive based on hash.
    local archive_hash=$(md5sum "$tarfile" | cut -d ' ' -f 1)
    if [[ -f $marker_file ]]
    then
      echo "$tarfile is newer than $marker_file, will check hash."
      if [[ $(cat "$marker_file") == "$archive_hash" ]]
      then
        touch "$marker_file"
        echo "Skipping unpack based on hash, already up-to-date: $tarfile"
        continue
      fi
    fi

    echo "Extracting .tar.gz: $tarfile - $archive_hash"
    # Remove old dir if needed
    output_dir_name=$(tar -tzf "$tarfile" | cut -f1 -d"/" | sort | uniq | head -n 1)
    output_dir_full_path="$REDIST/$output_dir_name"
    [[ -d "$output_dir_full_path" ]] && rm -rf "$output_dir_full_path"

    # Remove old marker if needed
    rm -f "$marker_file"

    # Unpack
    tar xzf "$tarfile" -C "$REDIST" || exitFailure 1 "Not a valid input .tar.gz"

    # Create marker
    echo "$archive_hash" > "$marker_file"
  done
  shopt -u nullglob
}

function unpack_zips()
{
  shopt -s nullglob
  for zipfile in $FETCHED_INPUTS_DIR/*.zip; do
    local archive_basename=$(basename "$zipfile")
    local marker_file="$REDIST/$archive_basename-marker"
    if should_skip_based_on_date "$zipfile"
    then
      echo "Skipping unpack based on date, already up-to-date: $zipfile"
      continue
    fi

    # If archive is newer than marker then check if we need to unpack archive based on hash.
    local archive_hash=$(md5sum "$zipfile" | cut -d ' ' -f 1)
    if [[ -f $marker_file ]]
    then
      echo "$zipfile is newer than $marker_file, will check hash."
      if [[ $(cat "$marker_file") == "$archive_hash" ]]
      then
        touch "$marker_file"
        echo "Skipping unpack based on hash, already up-to-date: $zipfile"
        continue
      fi
    fi

    echo "Extracting .zip: $zipfile - $archive_hash"
    # Remove old dir if needed
    output_dir_name=$(unzip -l "$zipfile" | tail -n +4 | head -n -2 | tr -s ' ' |  cut -d ' ' -f5 | cut -f1 -d"/" | sort | uniq | head -n 1)
    output_dir_full_path="$REDIST/$output_dir_name"
    [[ -d "$output_dir_full_path" ]] && rm -rf "$output_dir_full_path"

    # Remove old marker if needed
    rm -f "$marker_file"

    # Unpack
    unzip -qo "$zipfile" -d "$REDIST" || exitFailure 1 "Not a valid input .zip"

    # Create marker
    echo "$archive_hash" > "$marker_file"
  done
   shopt -u nullglob
}

unpack_tars
unpack_gzipped_tars
unpack_zips

if [[ -d $BASEDIR/tests/googletest ]]
then
  echo "Skipping copy, already present: $BASEDIR/tests/googletest"
else
  echo "Copying google test into place"
  cp -r $REDIST/googletest-release-1.8.1 $BASEDIR/tests/googletest
fi

echo "Finished unpacking"
