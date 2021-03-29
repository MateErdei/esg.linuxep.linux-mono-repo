#!/usr/bin/env python

## pip install -U boto
import boto
import boto.ec2

import os
import sys


ec2 = boto.ec2.connect_to_region("eu-west-1",
        aws_access_key_id="AKIAIF23TRE42IG5IH4Q",
        aws_secret_access_key="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")

volumes = ec2.get_all_volumes(filters={
    "status" : "available"
    })

for volume in volumes:
    print volume
    volume.delete()

