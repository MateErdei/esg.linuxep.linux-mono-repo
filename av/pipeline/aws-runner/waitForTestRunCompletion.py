#!/usr/bin/env python
from __future__ import absolute_import, print_function, division, unicode_literals

import json
import os
import glob
import sys
import time

import boto
import boto.exception
import boto.ec2

import boto.s3.key
from boto.s3.connection import S3Connection


TIMEOUT_FOR_ALL_TESTS = 100*60  # seconds


def checkMachinesAllTerminated(stack, uuid, start):
    conn = boto.ec2.connect_to_region(
        "eu-west-1",
        aws_access_key_id="AKIAWR523TF7XZPL2C7H",
        aws_secret_access_key="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"
        )
    instances = conn.get_only_instances(filters={
        "tag:TestPassUUID": uuid,
        "tag:TestImage": "true"
        })
    for instance in instances:
        if instance.state != "terminated":
            duration = time.time() - start
            minutes = duration // 60
            seconds = duration % 60
            print("Checking instance %s %s-%s ip %s after %d:%d" % (
                instance.id,
                instance.tags.get('Name', "<unknown>"),
                instance.tags.get('Slice', "<unknown-slice>"),
                instance.ip_address,
                minutes,
                seconds
            ))
            return False

    return True


def main(argv):
    if len(argv) > 1:
        STACK = argv[1]
    else:
        STACK = os.environ['STACK']

    if len(argv) > 2:
        TEST_PASS_UUID = argv[2]
    else:
        TEST_PASS_UUID = os.environ['TEST_PASS_UUID']

    start = time.time()
    #  and check for machines running
    delay = 120
    while time.time() < start + TIMEOUT_FOR_ALL_TESTS:
        try:
            if checkMachinesAllTerminated(STACK, TEST_PASS_UUID, start):
                duration = time.time() - start
                minutes = duration // 60
                seconds = duration % 60
                print("All instances terminated after %d:%02d" % (minutes, seconds))
                return 0
        except SyntaxError:
            raise
        except Exception as ex:
            print("Got exception but carrying on", str(ex))

        if time.time() - start > 20*60:
            delay = 30

        time.sleep(delay)

    print("Giving up and deleting stack anyway")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
