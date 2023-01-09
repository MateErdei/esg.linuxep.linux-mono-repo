#!/usr/bin/env python

import os
import subprocess
import sys
import time

import waitForTestRunCompletion
import reprocess

TIMEOUT_FOR_ALL_TESTS = waitForTestRunCompletion.TIMEOUT_FOR_ALL_TESTS



def get_duration(start):
    duration = time.time() - start
    minutes = duration // 60
    seconds = duration % 60
    return "%02d:%02d" % (minutes, seconds)


def checkMachinesAllTerminated(uuid, start, dest):
    conn = waitForTestRunCompletion.connect_to_aws()
    for instance in waitForTestRunCompletion.generate_unterminated_instances(conn, uuid):
        hostname = "%s-%s" % (
            instance.tags.get('Name', "<unknown>"),
            instance.tags.get('Slice', "<unknown-slice>")
        )

        formation_log = os.path.join(dest, hostname+"-cloudFormationInit.log")

        duration = get_duration(start)

        if os.path.isfile(formation_log):
            print("Instance %s %s finished (have formation log %s), but not terminated, ip %s after %s" %
                  (
                      instance.id,
                      hostname,
                      formation_log,
                      instance.ip_address,
                      duration
                  ))
            # continue checking
        else:
            print("Checking instance %s %s ip %s after %s" % (
                instance.id,
                hostname,
                instance.ip_address,
                duration
            ))
            return False

    return True


def print_all_machines_still_running(uuid, start):
    conn = waitForTestRunCompletion.connect_to_aws()
    for instance in waitForTestRunCompletion.generate_unterminated_instances(conn, uuid):
        duration = get_duration(start)
        print("Instance still running: %s %s-%s ip %s after %s" % (
            instance.id,
            instance.tags.get('Name', "<unknown>"),
            instance.tags.get('Slice', "<unknown-slice>"),
            instance.ip_address,
            duration
        ))


def sync(src, dest):
    subprocess.run(["aws", "s3", "sync", src, dest])


def do_reprocess(dest):
    files = os.listdir(dest)
    files = [ f for f in files if f.endswith("-output.xml") ]
    for f in files:
        p = os.path.join(dest, f)
        reprocess.reprocess(p)


def main(argv):
    TEST_PASS_UUID = os.environ['TEST_PASS_UUID']
    print("Waiting for completion for TEST_PASS_UUID: %s" % TEST_PASS_UUID)
    src = argv[1]
    dest = argv[2]

    start = time.time()
    #  and check for machines running
    delay = 120
    while time.time() < start + TIMEOUT_FOR_ALL_TESTS:
        try:
            if checkMachinesAllTerminated(TEST_PASS_UUID, start, dest):
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
        do_reprocess(dest)
        delay_duration = end_delay - time.time()
        if delay_duration > 0:
            time.sleep(delay_duration)

    print("Giving up and deleting stack anyway")
    print_all_machines_still_running(TEST_PASS_UUID, start)
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
