#!/usr/bin/env python

import FileInfo
import sys


def main(argv):
    if len(argv) < 2:
        print("Error: no file specified.")
        return 1

    filename = argv[1]
    file_info = FileInfo.FileInfo(".", filename)
    print(file_info.m_sha256)


if __name__ == '__main__':
    sys.exit(main(sys.argv))