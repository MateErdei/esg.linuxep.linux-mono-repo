#!/usr/bin/env bash
set -ex
set -o pipefail

function failure
{
    local EXIT=$1
    shift
    echo "$@"
    exit $EXIT
}
[[ -n "$BASE" ]] || failure 1 "BASE not specified"
[[ -d "$BASE" ]] || failure 2 "$BASE doesn't exist"

[[ -n "${GENERATE_SDDS_IMPORT}" ]] || failure 3 "GENERATE_SDDS_IMPORT not exported"
[[ -f "${GENERATE_SDDS_IMPORT}" ]] || failure 4 "Can't find ${GENERATE_SDDS_IMPORT}"
[[ -n "${OUTPUT}" ]] || failure 5 "OUTPUT not exported"

[[ -n "${GENERATE_MANIFEST_DAT}" ]] || failure 6 "GENERATE_MANIFEST_DAT not exported"
[[ -f "${GENERATE_MANIFEST_DAT}" ]] || failure 7 "Can't find ${GENERATE_MANIFEST_DAT}"


INPUT=$BASE/input/telemetry/telemetry-config.json

[[ -f $1/telemetry/telemetry-config.json ]] && INPUT=$1/telemetry/telemetry-config.json

[[ -f $INPUT ]] || failure 11 "telemetry-config.json input not found at $INPUT"

VERSION=$(python ${BASE}/products/distribution/getReleasePackageVersion.py ${BASE}/build/release-package-telemetry-supplement.xml)
PRODUCT_VERSION=$(python ${BASE}/products/distribution/computeAutoVersion.py ${BASE} $VERSION Jenkinsfile_telemetry_supplement)

TELEMETRY_SUPPLEMENT_COMPONENT=${OUTPUT}/SDDS-SSPL-TELEMSUPP-COMPONENT
rm -rf ${TELEMETRY_SUPPLEMENT_COMPONENT}
mkdir -p ${TELEMETRY_SUPPLEMENT_COMPONENT}
cp -a $INPUT ${TELEMETRY_SUPPLEMENT_COMPONENT}

## Copy components and top-level files to a temp tree.
## Generate manifest.dat
TEMP=$BASE/temp
rm -rf "${TEMP}"
mkdir -p "${TEMP}"

cp -a ${TELEMETRY_SUPPLEMENT_COMPONENT}/* "${TEMP}/"


rm -f ${TEMP}/SDDS-Import.xml ${TEMP}/manifest.dat

python "${GENERATE_MANIFEST_DAT}" ${TEMP}

cp ${TEMP}/manifest.dat ${TELEMETRY_SUPPLEMENT_COMPONENT}/

## Generate SDDS-Import.xml
export SDDSTEMPLATE="TELEMSUPP_TEMPLATE"
export PRODUCT_LINE_ID=SSPLTELEMSUPP
export PRODUCT_NAME=SophosServerProtectionLinuxTelemetrySupplement
#export FEATURE_LIST=BASE
export DEFAULT_HOME_FOLDER=${PRODUCT_LINE_ID}
export PRODUCT_VERSION="${PRODUCT_VERSION}"
export PRODUCT_TYPE="Component"
export LONG_DESCRIPTION="Sophos Protection for Linux Telemetry Supplement"
export SHORT_DESCRIPTION="SSPL Telemetry Supplement"


python ${GENERATE_SDDS_IMPORT} ${TELEMETRY_SUPPLEMENT_COMPONENT}

[[ -f ${TELEMETRY_SUPPLEMENT_COMPONENT}/SDDS-Import.xml ]] || failure 6 "Failed to create SDDS-Import.xml in TELEMETRY_SUPPLEMENT_COMPONENT"
grep -q "<ProductType>Component</ProductType>" "${TELEMETRY_SUPPLEMENT_COMPONENT}/SDDS-Import.xml" || failure 7 "Wrong ProductType in SDDS-Import.xml"