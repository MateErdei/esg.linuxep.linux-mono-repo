#!/usr/bin/env python

## pip install -U boto
import boto
import boto.ec2

import os
import sys


ec2 = boto.ec2.connect_to_region("eu-west-1",
        aws_access_key_id="AKIAWR523TF7XZPL2C7H",
        aws_secret_access_key="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")

volumes = ec2.get_all_volumes(filters={
    "status" : "available"
    })

for volume in volumes:
    print volume
    volume.delete()

