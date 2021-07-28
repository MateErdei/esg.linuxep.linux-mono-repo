#!/bin/bash

if [ -z "$1" ]
then
    echo "usage: settags.sh <instaceid>"
    exit 1
fi

aws ec2 create-tags --resources "$1" \
    --tags \
        'Key=OwnerName,Value=UKDevLinuxDarwin' \
        'Key=OwnerEmail,Value=ukdevlinuxdarwin@sophos.com' \
        'Key=SupportEmail,Value=ukdevlinuxdarwin@sophos.com' \
        'Key=BusinessUnit,Value=esg' \
        'Key=CostCentre,Value=GB11601200' \
        'Key=Project,Value=EDR' \
        'Key=Application,Value=Manual' \
        'Key=Environment,Value=development' \
        'Key=AutomationExcluded,Value=true' \
        'Key=SecurityClassification,Value=none low'
