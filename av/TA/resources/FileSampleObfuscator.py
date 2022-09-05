#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2022 Sophos Ltd
# All rights reserved.

import sys

XOR = b"password123"


def xor(data, pos):
    result = []
    for i in range(len(data)):
        result.append(data[i] ^ XOR[pos])
        pos += 1
        pos = pos % len(XOR)
    return pos, bytes(result)


def main(argv):
    src = argv[1]
    dest = argv[2]

    pos = 0

    with open(dest, "wb") as outFile:
        with open(src, "rb") as srcFile:
            while True:
                data = srcFile.read(10000)
                if len(data) == 0:
                    break
                pos, data = xor(data, pos)
                outFile.write(data)

    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
