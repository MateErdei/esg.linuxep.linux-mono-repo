#!/bin/bash
# Script used to as a wrapper to create a customer file listing multiple warehouses.

# Include libs
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null && pwd )"

pushd $SCRIPT_DIR

source SDDS.sh

targetDirectory=$1
updateFromSophos=$2
warehouses=$3

if [[ ${updateFromSophos} = "1" ]]
then
    createNewMultiWarehouseCustomerFile ${targetDirectory} "defaultUrl" ${warehouses[@]}
else
    createNewMultiWarehouseCustomerFileForUpdateCache ${targetDirectory} ${warehouses[@]}
fi

popd



