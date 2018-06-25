#!/usr/bin/env python

from __future__ import print_function,division,unicode_literals

import os
import sys

fullpath = os.path.join(os.getcwd(),__file__)
basedir = os.path.dirname(fullpath)
versig_test = os.path.dirname(basedir)
test = os.path.dirname(versig_test)
abs_srcdir = os.path.dirname(test)
bin_dir = os.path.join(abs_srcdir,"bin")

sys.path.append(os.path.join(bin_dir,"build"))

import Signing

class Options(object):
    pass

def main(argv):
    manifest = argv[1]
    files = argv[2:]

    options = Options()
    options.sign = True
    options.signing_oracle = "http://buildsign-m.eng.sophos:8000"

    signer = Signing.SignerBase(options)

    signer.signPackage(basedir,files,manifest,
        contentsPrefix="")

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
