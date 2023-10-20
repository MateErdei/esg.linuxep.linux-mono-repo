#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

import os
import re

from enum import Enum

from robot.api import logger
from robot.libraries.BuiltIn import BuiltIn


CAPNP_DIR = "/opt/test/inputs/av/test-resources/capnp-files"


def _migrate_capnp():
    TEST_INPUT_PATH = BuiltIn().get_variable_value("${TEST_INPUT_PATH}", "/opt/test/inputs")
    TAP_TEST_OUTPUT = TEST_INPUT_PATH + "/tap_test_output"
    CAPNP_SRC_DIR = TAP_TEST_OUTPUT

    if not os.path.isdir(CAPNP_SRC_DIR):
        logger.error(f"Capnp not found at {CAPNP_SRC_DIR}")
        return
    # Remove namespacing
    import_pattern = re.compile(r'using\s+Cxx\s*=\s*import\s+"/?capnp/c\+\+\.capnp";\s*\$Cxx\.namespace\(".*::.*"\);')

    os.makedirs(CAPNP_DIR, 0o755, exist_ok=True)
    for file in os.listdir(CAPNP_SRC_DIR):
        if not file.endswith(".capnp"):
            continue

        src = os.path.join(CAPNP_SRC_DIR, file)
        with open(src, 'r') as f:
            contents = f.read()

        dest = os.path.join(CAPNP_DIR, file)
        new_contents = import_pattern.sub(repl="", string=contents, count=1)
        if contents == new_contents and "Cxx" in contents:
            logger.error(f"Failed to remove Cxx from capnp for {file}")
        with open(dest, 'w') as f:
            f.write(new_contents)


class CapnpSchemas(Enum):
    NamedScan = "namedscan"
    ScanRequest = "scanrequest"
    ScanResponse = "scanresponse"
    ThreatDetected = "threatdetected"


class CapnpHelper:
    ROBOT_LIBRARY_SCOPE = 'GLOBAL'
    
    def __init__(self):
        self.__schema_object_map = None

    def __load_schema_if_required(self):
        if self.__schema_object_map is not None:
            return

        import capnp
        capnp.remove_import_hook()
        _migrate_capnp()

        try:
            # set up map of schemas to the capnp object
            named_scan_schema = capnp.load(f"{CAPNP_DIR}/NamedScan.capnp").NamedScan
            scan_request_schema = capnp.load(f"{CAPNP_DIR}/ScanRequest.capnp").FileScanRequest
            scan_response_schema = capnp.load(f"{CAPNP_DIR}/ScanResponse.capnp").FileScanResponse
            threat_detected_schema = capnp.load(f"{CAPNP_DIR}/ThreatDetected.capnp").ThreatDetected
            self.__schema_object_map = {CapnpSchemas.NamedScan: named_scan_schema,
                                        CapnpSchemas.ScanRequest: scan_request_schema,
                                        CapnpSchemas.ScanResponse: scan_response_schema,
                                        CapnpSchemas.ThreatDetected: threat_detected_schema}
        except OSError:
            logger.error("Unable to load capnp definitions")
            self.__schema_object_map = {}

    def check_named_scan_object(self, object_filename,
                                name: str = None,
                                scan_archives: bool = None,
                                scan_all_files: bool = None,
                                exclude_paths: list = None,
                                sophos_extension_exclusions: list = None,
                                user_defined_extension_inclusions: list = None,
                                scan_files_with_no_extensions: bool = None,
                                scan_hard_drives: bool = None,
                                scan_cd_dvd_drives: bool = None,
                                scan_network_drives: bool = None,
                                scan_removable_drives: bool = None,
                                detect_puas: bool = None):

        actual_named_scan = self._get_capnp_object(object_filename, CapnpSchemas.NamedScan)

        expected_values = {"name": name,
                           "excludePaths": exclude_paths,
                           "sophosExtensionExclusions": sophos_extension_exclusions,
                           "userDefinedExtensionInclusions": user_defined_extension_inclusions,
                           "scanArchives": scan_archives,
                           "scanAllFiles": scan_all_files,
                           "scanFilesWithNoExtensions": scan_files_with_no_extensions,
                           "scanHardDrives": scan_hard_drives,
                           "scanCDDVDDrives": scan_cd_dvd_drives,
                           "scanNetworkDrives": scan_network_drives,
                           "scanRemovableDrives": scan_removable_drives,
                           "detectPUAs": detect_puas}

        self._assert_schema_equal(CapnpSchemas.NamedScan,
                                  actual_named_scan,
                                  CapnpHelper._remove_none_values_from_dictionary(expected_values))
        return True

    def check_scan_request_object(self, object_filename, pathname: str = None, scan_inside_archives: bool = None):
        actual_named_scan = self._get_capnp_object(object_filename, CapnpSchemas.ScanRequest)

        expected_values = {"pathname": pathname,
                           "scanInsideArchives": scan_inside_archives}

        self._assert_schema_equal(CapnpSchemas.ScanRequest,
                                  actual_named_scan,
                                  CapnpHelper._remove_none_values_from_dictionary(expected_values))
        return True

    def check_scan_response_object(self, object_filename,
                                   clean: bool = None,
                                   threat_name: str = None,
                                   full_scan_result: str = None):
        actual_named_scan = self._get_capnp_object(object_filename, CapnpSchemas.ScanResponse)

        expected_values = {"clean": clean,
                           "threatName": threat_name,
                           "fullScanResult": full_scan_result}

        self._assert_schema_equal(CapnpSchemas.ScanResponse,
                                  actual_named_scan,
                                  CapnpHelper._remove_none_values_from_dictionary(expected_values))
        return True

    def check_threat_detected_object(self, object_filename,
                                     user_id: str = None,
                                     detection_time: int = None,
                                     threat_type: int = None,
                                     threat_name: str = None,
                                     scan_type: int = None,
                                     notification_status: int = None,
                                     file_path: str = None,
                                     action_code: int = None):

        actual_named_scan = self._get_capnp_object(object_filename, CapnpSchemas.ThreatDetected)

        expected_values = {"userID": user_id,
                           "detectionTime": detection_time,
                           "threatType": threat_type,
                           "threatName": threat_name,
                           "scanType": scan_type,
                           "notificationStatus": notification_status,
                           "filePath": file_path,
                           "actionCode": action_code}

        self._assert_schema_equal(CapnpSchemas.ThreatDetected,
                                  actual_named_scan,
                                  CapnpHelper._remove_none_values_from_dictionary(expected_values))
        return True

    def _assert_schema_equal(self, schema, actual, expected_scan_dictionary):
        self.__load_schema_if_required()
        expected = self.__schema_object_map[schema].new_message(**expected_scan_dictionary)

        for key in expected_scan_dictionary.keys():
            if not getattr(actual, key).__eq__(getattr(expected, key)):
                raise AssertionError(f"Object is not as expected\n"
                                     f"Actual: {actual}\n"
                                     f"Expected values: {expected}")

    def _get_capnp_object(self, object_filename, schema_enum):
        self.__load_schema_if_required()
        # takes object and loads it with the specified schema
        with open(object_filename, 'rb') as f:
            capnp_object = self.__schema_object_map[schema_enum].read(f)
        return capnp_object

    def _create_namedscan_capnp_object(self):
        self.__load_schema_if_required()
        message = self.__schema_object_map[CapnpSchemas.NamedScan].new_message()
        with open('/tmp/namedscan.bin', 'w+b') as f:
            message.write(f)

    @staticmethod
    def _remove_none_values_from_dictionary(dictionary):
        return {
            key: value
            for key, value in dictionary.items()
            if value is not None
        }
