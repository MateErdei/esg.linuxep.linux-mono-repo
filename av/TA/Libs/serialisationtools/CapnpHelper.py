#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

from enum import Enum
import capnp
import NamedScan_capnp

from .NamedScanHelper import NamedScanComparator


class CapnpSchemas(Enum):
    NamedScan = "namedscan"


class CapnpHelper:
    def __init__(self):
        # set up map of schemas
        named_scan_schema = NamedScan_capnp.NamedScan
        self.schema_map = {CapnpSchemas.NamedScan: named_scan_schema}

    def check_named_scan_name(self, object_filename, scan_name):
        named_scan = CapnpHelper._get_capnp_object(self, object_filename, CapnpSchemas.NamedScan)
        return named_scan.name == scan_name

    def check_named_scan_object(self, object_filename, **kwargs):
        actual_named_scan = CapnpHelper._get_capnp_object(self, object_filename, CapnpSchemas.NamedScan)
        test_object = NamedScanComparator(actual_named_scan, kwargs)
        test_object.assert_equal()
        return True

    def _get_capnp_object(self, object_filename, schema_enum):
        # takes object and loads it with the specified schema
        with open(object_filename, 'rb') as f:
            capnp_object = self.schema_map[schema_enum].read(f)
        return capnp_object

    def _create_namedscan_capnp_object(self):
        message = self.schema_map[CapnpSchemas.NamedScan].new_message()
        with open('/tmp/namedscan.bin', 'w+b') as f:
            message.write(f)
