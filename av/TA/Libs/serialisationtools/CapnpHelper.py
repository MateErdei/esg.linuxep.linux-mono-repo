#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

from enum import Enum
import capnp
import os

CAPNP_FILE_PATHS = "/opt/test/inputs/test_scripts/resources/capnp-files/"


class CapnpSchemas(Enum):
    NamedScan = "namedscan"
    ScanRequest = "scanrequest"
    ScanResponse = "scanresponse"
    ThreatDetected = "threatdetected"


class CapnpHelper:
    def __init__(self):
        capnp.remove_import_hook()

        # set up map of schemas to the capnp object
        named_scan_schema = capnp.load(f"{CAPNP_FILE_PATHS}NamedScan.capnp").NamedScan
        scan_request_schema = capnp.load(f"{CAPNP_FILE_PATHS}ScanRequest.capnp").FileScanRequest
        scan_response_schema = capnp.load(f"{CAPNP_FILE_PATHS}ScanResponse.capnp").FileScanResponse
        threat_detected_schema = capnp.load(f"{CAPNP_FILE_PATHS}ThreatDetected.capnp").ThreatDetected
        self.schema_object_map = {CapnpSchemas.NamedScan: named_scan_schema,
                                  CapnpSchemas.ScanRequest: scan_request_schema,
                                  CapnpSchemas.ScanResponse: scan_response_schema,
                                  CapnpSchemas.ThreatDetected: threat_detected_schema}

    def check_named_scan_object(self, object_filename, **kwargs):
        actual_named_scan = CapnpHelper._get_capnp_object(self, object_filename, CapnpSchemas.NamedScan)
        self.assert_schema_equal(actual_named_scan, kwargs)
        return True

    def assert_schema_equal(self, actual, expected_scan_dictionary):
        actual_object = actual.to_dict()
        if not all(elem in expected_scan_dictionary.keys() for elem in actual_object.keys()):
            raise AssertionError(f"Object is not as expected\n"
                                 f"Actual: {actual_object}\n"
                                 f"Expected values: {expected_scan_dictionary}")

        for entry in expected_scan_dictionary.items():
            if actual_object[entry[0]] != entry[1]:
                raise AssertionError(f"Object is not as expected\n"
                                     f"Actual: {actual_object}\n"
                                     f"Expected values: {expected_scan_dictionary}")
        pass

    def _get_capnp_object(self, object_filename, schema_enum):
        # takes object and loads it with the specified schema
        with open(object_filename, 'rb') as f:
            capnp_object = self.schema_object_map[schema_enum].read(f)
        return capnp_object

    def _create_namedscan_capnp_object(self):
        message = self.schema_object_map[CapnpSchemas.NamedScan].new_message()
        with open('/tmp/namedscan.bin', 'w+b') as f:
            message.write(f)
