#!/bin/env python

import os
import sys
import subprocess

def output(lines):
    cert = "\n".join(lines)
    proc = subprocess.Popen(['openssl','x509','-noout','-subject'],stdin=subprocess.PIPE)
    proc.communicate(cert)
    proc.wait()

currentCert = []

for line in open(sys.argv[1]).readlines():
    line = line.strip()
    if line == "-----BEGIN CERTIFICATE-----":
        currentCert = [line]
    elif line == "-----END CERTIFICATE-----":
        currentCert.append(line)
        output(currentCert)
    else:
        currentCert.append(line)

