#!/usr/bin/python

from copyLocalBuildOutputToDevWarehouse import BASE, MTR

def main():
    BASE.setup_host_branch_directory()
    MTR.setup_host_branch_directory()

if __name__ == '__main__':
    main()