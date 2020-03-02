#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

from enum import Enum


class NamedScanAttributes(Enum):
    Name = "name"
    ExcludePaths = "excludePaths"
    SophosExtensionExclusions = "sophosExtensionExclusions"
    UserDefinedExtensionInclusions = "userDefinedExtensionInclusions"
    ScanArchives = "scanArchives"
    ScanAllFiles = "scanAllFiles"
    ScanFilesWithNoExtensions = "scanFilesWithNoExtensions"
    ScanHardDrives = "scanHardDrives"
    ScanCDDVDDrives = "scanCDDVDDrives"
    ScanNetworkDrives = "scanNetworkDrives"
    ScanRemovableDrives = "scanRemovableDrives"


class NamedScanComparator:
    def __init__(self, actual, expected_scan_dictionary):
        self.actual_object = actual.to_dict()
        self.expected = {}
        for entry in expected_scan_dictionary:
            if entry[0] in NamedScanAttributes:
                self.expected[entry[0]] = entry[1]

    def assert_equal(self):
        for entry in self.expected.items():
            if self.actual_object[entry[0]] != entry[1]:
                raise AssertionError("Scan config is not as expected\nActual: ", self.actual_object,
                                     "\nExpected: ", self.expected)
        pass
