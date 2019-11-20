#!/usr/bin/python

import copyToDevWarehouse
from copyToDevWarehouse import HOSTNAME, BFR_PATH, SSPL_TOOLS, BASE, MTR


def error(errorCode, message):
    assert type(errorCode) == int
    print message
    return errorCode

def main():
    BASE.setup_host_branch_directory()
    MTR.setup_host_branch_directory()

if __name__ == '__main__':
    main()