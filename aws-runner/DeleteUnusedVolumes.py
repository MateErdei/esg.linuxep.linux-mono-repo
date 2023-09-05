#!/usr/bin/env python3

## pip install -U boto
import boto
import boto.ec2

import os
import sys

# Use credentials from environment
ec2 = boto.ec2.connect_to_region("eu-west-1")

volumes = ec2.get_all_volumes(filters={
    "status": "available"
    })

for volume in volumes:
    print(volume)
    try:
        volume.delete()
    except Exception as ex:
        print("Failed to delete %s: %s" % (volume, str(ex)))

