#!/bin/bash
set -ex
SDDS3_BUILDER=$1
PACKAGE_DIR=$2
SDDS_IMPORT=$3
NONCE=$(sha256sum $SDDS_IMPORT | head -c 10)
chmod +x $SDDS3_BUILDER
echo $SDDS3_BUILDER --build-package --package-dir "$PACKAGE_DIR" --sdds-import "$SDDS_IMPORT" --nonce $NONCE
$SDDS3_BUILDER --build-package --package-dir "$PACKAGE_DIR" --sdds-import "$SDDS_IMPORT" --nonce $NONCE
