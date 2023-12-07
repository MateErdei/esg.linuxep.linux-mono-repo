#!/bin/bash

# Uncomment for debugging
#set -x

declare -a PYPI_PKGS=("pycryptodome"
                       "certifi"
                       "chardet"
                       "idna"
                       "pathtools"
                       "requests"
                       "six"
                       "sseclient"
                       "urllib3"
                       "watchdog")

set -e
CLEAN=0
while [[ $# -ge 1 ]]
do
    case $1 in
        --clean)
            CLEAN=1
            ;;
        *)
            exitFailure $FAILURE_BAD_ARGUMENT "unknown argument $1"
            ;;
    esac
    shift
done

function get_lock()
{
  echo "Getting lock, so only one unpack can run at a time..."
  # Using redirection applied to the current shell (exec with no command)
  # open a file and return the file descriptor (https://www.gnu.org/software/bash/manual/html_node/Redirections.html)
  exec 3>unpack_lock_file
  # lock fd 3, with a timeout of 120 seconds
  flock -w 120 -x 3
  echo "Got lock"
}

function unlock()
{
  exec 3>&-
  rm -f unpack_lock_file
}

get_lock

## This had to be commented out for Continuous Fuzzer fix. Still desirable code if we can improve the Fuzzer impl.
#if [[ "$CI" == "true" ]]
#  then
#    echo "Building in CI, allowing root user execution."
#  else
#    if [[ $(id -u) == 0 ]]
#    then
#        echo "You don't need to run this as root"
#        exit 1
#    fi
#fi

BASEDIR=$(dirname "$0")

# Just in case this script ever gets symlinked
BASEDIR=$(readlink -f "$BASEDIR")
cd "$BASEDIR"

if [[ "$LINUX_ENV_SETUP" != "true" ]]
then
  source "$BASEDIR/setup_env_vars.sh"
fi

if [[ -z ${ROOT_LEVEL_BUILD_DIR+x} ]]
then
  echo "ROOT_LEVEL_BUILD_DIR is not set"
  echo "Please run: . ./setup_env_vars.sh"
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
  echo "Please run: . ./setup_env_vars.sh"
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
  echo "Please run: . ./setup_env_vars.sh"
  exit 1
fi

if [[ "$CLEAN" == "1" ]]
then
  rm -rf "$REDIST"
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
  local archive_basename
  archive_basename=$(basename "$archive")
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
    local archive_basename
    archive_basename=$(basename "$tarfile")
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
      local output_dir_name
      output_dir_name=$(tar -tf "$tarfile" | cut -f1 -d"/" | sort | uniq | head -n 1)
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

function unpack_gzipped_tar()
{
  tarfile="$1"
  dest_dir="$2"
  optional_args="$3"
  local archive_basename
  archive_basename=$(basename "$tarfile")
  local marker_file="$dest_dir/$archive_basename-marker"
  if should_skip_based_on_date "$tarfile"
  then
    echo "Skipping unpack based on date, already up-to-date: $tarfile"
    return
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
      return
    fi
  fi

  echo "Extracting .tar.gz: $tarfile - $archive_hash"
  # Remove old dir if needed
  output_dir_name=$(tar -tzf "$tarfile" | cut -f1 -d"/" | sort | uniq | head -n 1)
  output_dir_full_path="$dest_dir/$output_dir_name"
  [[ -d "$output_dir_full_path" ]] && rm -rf "$output_dir_full_path"

  # Remove old marker if needed
  rm -f "$marker_file"

  # Unpack
  tar xzf "$tarfile" $optional_args -C "$dest_dir" || exitFailure 1 "Not a valid input .tar.gz"

  # Create marker
  echo "$archive_hash" > "$marker_file"
}

function unpack_gzipped_tars()
{
  shopt -s nullglob
  for tarfile in "$FETCHED_INPUTS_DIR/"*.tar.gz; do
    unpack_gzipped_tar $tarfile $REDIST
  done
  shopt -u nullglob
}

function unpack_zip()
{
  zipfile="$1"
  dest_dir="$2"
  local archive_basename
  archive_basename=$(basename "$zipfile")
  local marker_file="$dest_dir/$archive_basename-marker"
  if should_skip_based_on_date "$zipfile"
  then
    echo "Skipping unpack based on date, already up-to-date: $zipfile"
    return
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
      return
    fi
  fi

  echo "Extracting .zip: $zipfile - $archive_hash"
  # Remove old dir if needed
  output_dir_name=$(unzip -l "$zipfile" | tail -n +4 | head -n -2 | tr -s ' ' |  cut -d ' ' -f5 | cut -f1 -d"/" | sort | uniq | head -n 1)
  output_dir_full_path="$dest_dir/$output_dir_name"
  [[ -d "$output_dir_full_path" ]] && rm -rf "$output_dir_full_path"

  # Remove old marker if needed
  rm -f "$marker_file"

  # Unpack
  unzip -qo "$zipfile" -d "$dest_dir" || exitFailure 1 "Not a valid input .zip"

  # Create marker
  echo "$archive_hash" > "$marker_file"
}

function unpack_zips()
{
  shopt -s nullglob
  for zipfile in $FETCHED_INPUTS_DIR/*.zip; do
    unpack_zip $zipfile $REDIST
  done
  shopt -u nullglob
}

function unpack_pypi_pkgs()
{
  shopt -s nullglob
  for pkg_name in ${PYPI_PKGS[@]}
  do
    pkg_path=$(ls "$FETCHED_INPUTS_DIR/pypi/${pkg_name}"*)
    dest_dir="${REDIST}/pypi/"
    mkdir -p $dest_dir
    if [[ $pkg_path == *.whl ]]
    then
      echo "Extracting wheel file: $pkg_path to $dest_dir"
      unpack_zip "$pkg_path" "$dest_dir"
    elif [[ $pkg_path == *.tar.gz ]]
    then
      echo "Extracting gzipped tar file: $pkg_path to $dest_dir"
      unpack_gzipped_tar "$pkg_path" "$dest_dir" "--strip-components 1"
    else
      echo "Unexpected file extension, ignoring: $pkg_path"
    fi
  done
  shopt -u nullglob
}


function setup_cmake()
{
    if [[ -f "$FETCHED_INPUTS_DIR/cmake/bin/cmake" ]]
    then
        if [[ ! -d $REDIST/cmake ]]
        then
            ln -sf $FETCHED_INPUTS_DIR/cmake $REDIST/cmake
        fi
    else
        echo "ERROR - cmake not found here: $FETCHED_INPUTS_DIR/cmake/bin/cmake"
        exit 1
    fi
    chmod 700 $REDIST/cmake/bin/cmake || exitFailure "Unable to chmod cmake"
    chmod 700 $REDIST/cmake/bin/ctest || exitFailure "Unable to chmod ctest"
    echo "cmake synced to $REDIST/cmake"
}

function apply_third_party_patches()
{
  # This patch allows us to know when sseclient has lost a connection
  # We catch the StopIteration exception in mcs_push_client.py and handle it
  if [ ! -f ${REDIST}/pypi/sseclient_patched ]
  then
    touch ${REDIST}/pypi/sseclient_patched
    patch ${REDIST}/pypi/sseclient.py < build/sseclient.patch
  fi
}

unpack_tars
unpack_gzipped_tars
unpack_zips
unpack_pypi_pkgs
apply_third_party_patches
setup_cmake

unlock
echo "Finished unpacking"
