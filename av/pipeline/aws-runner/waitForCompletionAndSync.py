#!/usr/bin/env python

import os
import subprocess
import sys
import time

import waitForTestRunCompletion

TIMEOUT_FOR_ALL_TESTS = waitForTestRunCompletion.TIMEOUT_FOR_ALL_TESTS
checkMachinesAllTerminated = waitForTestRunCompletion.checkMachinesAllTerminated


def sync(src, dest):
    subprocess.run(["aws", "s3", "sync", src, dest])


def main(argv):
    STACK = argv[1]
    TEST_PASS_UUID = os.environ['TEST_PASS_UUID']
    src = argv[2]
    dest = argv[3]

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

        end_delay = time.time() + delay
        sync(src, dest)
        delay_duration = end_delay - time.time()
        if delay_duration > 0:
            time.sleep(delay_duration)

    print("Giving up and deleting stack anyway")
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
