SDDS_DIR="$(cd "$(dirname -- "$BASH_SOURCE")"; pwd -P)"
export OVERRIDE_SUS_LOCATION=https://localhost:8080
export OVERRIDE_CDN_LOCATION=https://localhost:8080
export OVERRIDE_SOPHOS_CERTS="$SDDS_DIR/../base/testUtils/SupportFiles/dev_certs"
