#!/usr/bin/env python3
# Copyright 2019 Sophos Plc, Oxford, England.

"""
event_receiver Module
"""



import logging
import os
import re
import shutil

import mcsrouter.utils.path_manager as path_manager
import mcsrouter.utils.xml_helper as xml_helper

LOGGER = logging.getLogger(__name__)


def receive():
    """
    Async receive call
    """

    event_dir = os.path.join(path_manager.event_dir())
    for event_file in os.listdir(event_dir):
        match_object = re.match(r"([A-Z]*)_event-(.*).xml", event_file)
        file_path = os.path.join(event_dir, event_file)
        if match_object:
            app_id = match_object.group(1)
            time = os.path.getmtime(file_path)
            body = xml_helper.get_escaped_non_ascii_content(
                file_path)
            try:
                xml_helper.check_xml_has_no_script_tags(body)
            except xml_helper.XMLException as ex:
                LOGGER.error("Failed verification of XML as it contains script tags. Error: {}".format(str(ex)))
                continue

            event_file = os.path.basename(file_path)
            if event_file.startswith("ALC"):
                handle_alc_event(file_path)
            else:
                os.remove(file_path)

            yield (app_id, time, body)
        else:
            LOGGER.warning("Malformed event file: %s", event_file)
            os.remove(file_path)


def handle_alc_event(file_path):

    for filename in os.listdir(path_manager.event_cache_dir()):
        old_file_path = os.path.join(path_manager.event_cache_dir(), filename)
        try:
            if os.path.isfile(old_file_path) or os.path.islink(old_file_path):
                os.unlink(old_file_path)
        except Exception as ex:
            LOGGER.error('Failed to delete file {} due to error {}'.format(old_file_path, str(ex)))

    cache_path = os.path.join(path_manager.event_cache_dir(), os.path.basename(file_path))
    shutil.move(file_path, cache_path)

    LOGGER.debug("Moved file {} to {}".format(file_path, cache_path))