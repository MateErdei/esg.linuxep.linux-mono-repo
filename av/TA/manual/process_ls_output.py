#!/bin/env python3
from __future__ import absolute_import, print_function, division, unicode_literals

import re
import sys

DIR_RE = re.compile(r"^(\S+):$")
FILE_RE = re.compile(r"^([-prwxdsXT]{10,11}) +\d+ +([-\w]+) +([-\w]+) +\d+ \w{3} +\d{1,2} +\d{2}:?\d{2} +([.()\S ]+)$")
FILE_YEAR_RE = re.compile(r"^([-rwxdsX]{10}) +\d+ +([-\w]+) +([-\w]+) +\d+ \w{3} +\d{1,2} +\d{4} +([.\S]+)$")
SYMLINK_RE = \
    re.compile(r"^([-lrwxdsX]{10}) +\d+ +([-\w]+) +([-\w]+) +\d+ \w{3} +\d+ +\d{2}:?\d{2} +([.\S]+) -> [.\S/]+$")

DISTRIBUTION_VERSION_RE = re.compile(r"distribution_version/[0-9a-f]+")

def read_file(src):
    results = {}
    directory = None
    for line in open(src).readlines():
        line = line.strip()
        if len(line) == 0 or line.startswith("total "):
            continue
        mo = DIR_RE.match(line)
        if mo:
            directory = mo.group(1)
            # tidy distribution_version/31192e1650dc6416e1843181a358907b680070b76e76569f39eb424bc933234d
            directory = DISTRIBUTION_VERSION_RE.sub("distribution_version/<VERSION>", directory)
            continue

        mo = FILE_RE.match(line)
        if not mo:
            mo = FILE_YEAR_RE.match(line)
        if not mo:
            mo = SYMLINK_RE.match(line)

        if mo:
            perm = mo.group(1)
            user = mo.group(2)
            group = mo.group(3)
            p = directory + "/" + mo.group(4)
            results[p] = (perm, user, group)
            continue

        print(line, "failed to match!")
        raise Exception("Failed to match %s" % line)

    return results

def dump_results(results, dest):
    EXCLUDED = (
        "/opt/sophos-spl/plugins/av/var/customer_id.txt",
        "/opt/sophos-spl/plugins/av/chroot/opt/sophos-spl/plugins/av/var/customer_id.txt"
    )
    for key in sorted(results.keys()):
        if key.startswith("/opt/sophos-spl/plugins/av/lib64/libstdc++.so.6"):
            continue
        if key in EXCLUDED:
            continue
        (perm, user, group) = results[key]
        print(key, perm, user, group)


def process_file(src, dest):
    results = read_file(src)
    dump_results(results, dest)

def main(argv):
    # symlink_re = re.compile(r"^([lrwxdsX-]{10}) +\d+ +([-\w]+) +([-\w]+) +\d+ Jun +\d+ \d{2}:\d{2} +([.\S]+) -> [.\S/]+$")
    # line = "lrwxrwxrwx 1 root sophos-spl-group     11 Jun  1 12:34 avscanner -> avscanner.0"
    # if not symlink_re.match(line):
    #     print("line not matching!")
    #     return 1
    return process_file(argv[1], None)


if __name__ == "__main__":
    sys.exit(main(sys.argv))
