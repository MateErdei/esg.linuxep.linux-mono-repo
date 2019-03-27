#!/bin/env python

import sys
import subprocess

def output(lines):
    cert = "\n".join(lines)
    proc = subprocess.Popen(['openssl','x509','-noout','-subject'],stdin=subprocess.PIPE)
    proc.communicate(cert)
    proc.wait()


for arg in sys.argv[1:]:
    currentCert = []
    print(arg)
    for line in open(arg).readlines():
        line = line.strip()
        if line == "-----BEGIN CERTIFICATE-----":
            currentCert = [line]
        elif line == "-----END CERTIFICATE-----":
            currentCert.append(line)
            output(currentCert)
        else:
            currentCert.append(line)
