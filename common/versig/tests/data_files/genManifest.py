#!/usr/bin/env python

from __future__ import print_function,division,unicode_literals

import os
import sys
import Signing


fullpath = os.path.join(os.getcwd(),__file__)
basedir = os.path.dirname(fullpath)


class Options(object):
    pass


def main(argv):
    manifest = argv[1]
    files = argv[2:]
    includeSHA256 = os.environ.get("SHA256", "1") == "1"

    options = Options()
    options.sign = True
    options.signing_oracle = "http://buildsign-m.eng.sophos:8000"

    signer = Signing.SignerBase(options)

    signer.signPackage(
        basedir,
        files,
        manifest,
        contentsPrefix="",
        includeSHA256=includeSHA256)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))
