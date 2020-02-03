#!/usr/bin/env python

# openssl x509 -subject_hash -fingerprint -noout -in <filename>

import os
import re
import sys
import subprocess

def rehash(d,f):
    print("Rehashing %s"%f)
    p = os.path.join(d,f)
    proc = subprocess.Popen(['openssl','x509','-subject_hash','-fingerprint','-noout','-in',p],
        stdout=subprocess.PIPE)

    stdout = proc.communicate()[0]

    lines = stdout.splitlines()
    h = lines[0].strip()
    fingerprint = lines[1].strip()

    dest = os.path.join(d,h+".0")
    if os.path.isfile(dest):
        os.unlink(dest)
    os.symlink(f,dest)

def main(argv):
    d = argv[1]
    tohash = []
    for f in os.listdir(d):
        p = os.path.join(d,f)
        if not os.path.isfile(p):
            continue

        if re.match(r"^[\da-f]+\.r?\d+$",f):
            os.unlink(p)
        elif f.endswith(".pem"):
            tohash.append(f)

    for f in tohash:
        rehash(d,f)

    return 0


if __name__=="__main__":
    sys.exit(main(sys.argv))
