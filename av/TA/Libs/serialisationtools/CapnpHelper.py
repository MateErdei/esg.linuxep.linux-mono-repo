#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

from enum import Enum

import capnp
from robot.api import logger

CAPNP_DIR = "/opt/test/inputs/av/test-resources/capnp-files"


class CapnpSchemas(Enum):
    NamedScan = "namedscan"
    ScanRequest = "scanrequest"
    ScanResponse = "scanresponse"
    ThreatDetected = "threatdetected"


class CapnpHelper:
    ROBOT_LIBRARY_SCOPE = 'GLOBAL'
    
    def __init__(self):
        capnp.remove_import_hook()

        try:
            # set up map of schemas to the capnp object
            named_scan_schema = capnp.load(f"{CAPNP_DIR}/NamedScan.capnp").NamedScan
            scan_request_schema = capnp.load(f"{CAPNP_DIR}/ScanRequest.capnp").FileScanRequest
            scan_response_schema = capnp.load(f"{CAPNP_DIR}/ScanResponse.capnp").FileScanResponse
            threat_detected_schema = capnp.load(f"{CAPNP_DIR}/ThreatDetected.capnp").ThreatDetected
            self.schema_object_map = {CapnpSchemas.NamedScan: named_scan_schema,
                                      CapnpSchemas.ScanRequest: scan_request_schema,
                                      CapnpSchemas.ScanResponse: scan_response_schema,
                                      CapnpSchemas.ThreatDetected: threat_detected_schema}
        except OSError:
            logger.error("Unable to load capnp definitions")
            self.schema_object_map = {}

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
                                scan_removable_drives: bool = None):

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
                           "scanRemovableDrives": scan_removable_drives}

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
        expected = self.schema_object_map[schema].new_message(**expected_scan_dictionary)

        for key in expected_scan_dictionary.keys():
            if not getattr(actual, key).__eq__(getattr(expected, key)):
                raise AssertionError(f"Object is not as expected\n"
                                     f"Actual: {actual}\n"
                                     f"Expected values: {expected}")

    def _get_capnp_object(self, object_filename, schema_enum):
        # takes object and loads it with the specified schema
        with open(object_filename, 'rb') as f:
            capnp_object = self.schema_object_map[schema_enum].read(f)
        return capnp_object

    def _create_namedscan_capnp_object(self):
        message = self.schema_object_map[CapnpSchemas.NamedScan].new_message()
        with open('/tmp/namedscan.bin', 'w+b') as f:
            message.write(f)

    @staticmethod
    def _remove_none_values_from_dictionary(dictionary):
        return {
            key: value
            for key, value in dictionary.items()
            if value is not None
        }
