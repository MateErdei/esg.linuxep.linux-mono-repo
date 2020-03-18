#!/bin/bash


export https_proxy=${https_proxy:-http://allegro.eng.sophos:3128}
export http_proxy=${http_proxy:-http://allegro.eng.sophos:3128}

aws ec2 create-tags --resources "$@" \
    --tags \
        'Key=OwnerName,Value=ukdevsavlinux' \
        'Key=OwnerEmail,Value=ukdevsavlinux@sophos.com' \
        'Key=SupportEmail,Value=ukdevsavlinux@sophos.com' \
        'Key=BusinessUnit,Value=esg' \
        'Key=CostCentre,Value=GB11601200' \
        'Key=Project,Value=SSPLAV' \
        'Key=Application,Value=SSPLAV' \
        'Key=Branch,Value=Manual' \
        'Key=Environment,Value=development' \
        'Key=AutomationExcluded,Value=true' \
        'Key=SecurityClassification,Value=none low'
