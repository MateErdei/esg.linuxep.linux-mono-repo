SSPL_WAREHOUSE_DIR="$(cd "$(dirname -- "$BASH_SOURCE")"; pwd -P)"
export OVERRIDE_SUS_LOCATION=https://localhost:8080
export OVERRIDE_CDN_LOCATION=https://localhost:8080
export OVERRIDE_SOPHOS_CERTS="$SSPL_WAREHOUSE_DIR/TA/SupportFiles/dev_certs"
