#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Plc, Oxford, England.
# All rights reserved.

from enum import Enum
import capnp
import NamedScan_capnp

class CapnpSchemas(Enum):
    NamedScan = "namedscan"


class CapnpHelper:
    schema_map = {}

    @staticmethod
    def setup():
        # set up map of schemas
        named_scan_schema = NamedScan_capnp.NamedScan
        CapnpHelper.schema_map = {CapnpSchemas.NamedScan: named_scan_schema}
        pass

    @staticmethod
    def get_capnp_object(object_filename, schema_enum):
        # takes object and loads it with the specified schema
        with open(object_filename, 'rb') as f:
            capnp_object = CapnpHelper.schema_map[schema_enum].read(f)
        return capnp_object

    @staticmethod
    def create_namedscan_capnp_object():
        message = CapnpHelper.schema_map[CapnpSchemas.NamedScan].new_message()
        with open('/tmp/namedscan.bin', 'w+b') as f:
            message.write(f)
