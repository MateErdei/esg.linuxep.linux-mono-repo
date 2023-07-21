#!/usr/bin/env python3

from __future__ import print_function,division,unicode_literals

import sys

import sb_manifest_sign.sb_manifest_sign


def main(argv):
    return sb_manifest_sign.sb_manifest_sign.main()


if __name__ == '__main__':
    sys.exit(main(sys.argv))
