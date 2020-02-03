#!/bin/bash

function printUsage()
{
	echo "Usage:"
	echo "./generateWarehouse.sh <Source Directory> <Target Directory> <Rigid Name>"
	echo "Source Directory: This is the directory that contains product files which you want to turn into a warehouse, these files will not be altered. They will be copied to <Target Directory>/product."
	echo "Target Directory: This is the directory that will contain the generated warehouse files and a copy of the product product files."
	echo "Rigid Name: This is the GUID or Rigid Name of the product in the warehouse. It is what SUL etc. use to identify a product."
}

function exitWithMessage()
{
    echo $1
    exit 1
}

function run()
{
	local warehouseName=$1
	local updateFromSophos=$2

	# Move to correct directory.
	parentPath=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
	pushd "$parentPath"

	# Include libs
	source SDDS.sh

	declare -a list_of_all_warehouse_components
	get_all_components_for_warehouse ${warehouseName}

    popd

    for component_record in ${list_of_all_warehouse_components[@]}
    do
        IFS=, read -a component_config_values <<< ${component_record[@]}

        rootSourceDirectory=${component_config_values[3]}
        rootTargetDirectory=${component_config_values[4]}
        rigidName=${component_config_values[2]}

        # Some paranoid checks to ensure we don't rm rf root dir.
        if [[ ${rootTargetDirectory} = "/" ]]; then
            printUsage
            exitWithMessage "Target directory set to /"
        fi
        if [[ ${rootTargetDirectory} = "" ]]; then
            printUsage
            exitWithMessage "Target directory set to empty string."
        fi
        if [ -z ${rootTargetDirectory+x} ]
        then
            printUsage
            exitWithMessage "Target directory not set"
        fi

        sourceDirectory=${rootSourceDirectory}/${rigidName}
        targetDirectory=${rootTargetDirectory}/${rigidName}
        echo "Generating warehouse from $sourceDirectory"
        echo "Saving warehouse to $targetDirectory"

        # Clean up target dir, it should be made empty at this stage.
        #rm -rf "$targetDirectory" || exitWithMessage "Could not delete target directory."

        mkdir -p "$targetDirectory"

        # Get absolute paths
        sourceDirectory=$(readlink -f $sourceDirectory)
        targetDirectory=$(readlink -f $targetDirectory)

        # Product directory that we can copy the product files to, needs to not exist before we do the copy.
        local sddsDir="$targetDirectory/sdds"

        rm -rfv "$sddsDir"
        cp -rv "$sourceDirectory" "$sddsDir"
        find "$sourceDirectory" "$sddsDir" -type f | xargs md5sum

    done

	# Move to correct directory.
	parentPath=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
	pushd "$parentPath"


	if [[ ${updateFromSophos} = "1" ]]
	then
	    requireSophosWarehousesHttps ${warehouseName} ${updateFromSophos}
	else
	    generateUpdateCacheWarehouse ${rootTargetDirectory} ${warehouseName}
	fi

	popd
	find "$rootSourceDirectory" "$sddsDir" -type f | xargs md5sum
}

export LANG=C
export LC_ALL=C

# Check if source is set.
warehouseName=$1
updateFromSophos=$2

# Check if warehouse name is set.
if [[ -z ${warehouseName} ]]
then
	printUsage
	exitWithMessage "Warehouse name not set"
fi

run ${warehouseName} ${updateFromSophos}


