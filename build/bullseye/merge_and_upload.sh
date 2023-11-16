#!/usr/bin/env bash
echo "Generating bullseye output"

script_dir="${0%/*}"

failure_bullseye=2
failure_private_key=3
failure_upload=4

function exitFailure()
{
    local code=$1
    shift
    echo "$@"
    exit "$code"
}

# TAP template bullseye is installed to /usr/local, but by default, Bullseye is installed to /opt/BullseyeCoverage
if [[ -f /opt/BullseyeCoverage/bin/covselect ]]
then
  BULLSEYE_DIR=/opt/BullseyeCoverage
elif [[ -f /usr/local/bin/covselect ]]
then
  BULLSEYE_DIR=/usr/local
else
  exitFailure $failure_bullseye "Failed to find bullseye"
fi

echo "Bullseye location: $BULLSEYE_DIR"

branch="$1"
shift
group="$1"
shift

output_dir="/opt/test/coverage/$group"
mkdir -p "$output_dir"
covfile="$output_dir/monorepo.cov"
htmldir="$output_dir/html"

"$BULLSEYE_DIR/bin/covmerge" --no-banner --create --file "$covfile" "$@"

echo "Exclusions:"
"$BULLSEYE_DIR/bin/covselect" --no-banner --list --file "$covfile"

"$BULLSEYE_DIR/bin/covhtml" \
    --file "$covfile"       \
    --verbose               \
    "$htmldir"              \
    </dev/null              \
    || exitFailure $failure_bullseye "Failed to generate bullseye html"

chmod -R a+rX "$htmldir"

echo "Uploading bullseye output to $group"

private_key="${script_dir}/private.key"
[[ -f ${private_key} ]] || exitFailure $failure_private_key "Unable to find private key for upload"

## Ensure ssh won't complain about private key permissions:
chmod 600 "${private_key}"
rsync -az --rsh="ssh -i ${private_key} -o StrictHostKeyChecking=no" --delete "${htmldir}/" \
    "upload@allegro.eng.sophos:public_html/bullseye/linux-mono-repo-${group}-${branch}/"  \
    </dev/null \
    || exitFailure $failure_upload "Failed to upload bullseye html"
