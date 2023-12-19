#!/bin/env python3

import sys

f = open(sys.argv[1], "w")
count = int(sys.argv[2])
for i in range(count):
    f.write(f"/A/{i}/*/tail\n")
f.close()

