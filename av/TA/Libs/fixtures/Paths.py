#!/bin/env python3

import os
import logging
logger = logging.getLogger("Paths")
logger.setLevel(logging.DEBUG)

def sophos_spl_path():
    return os.environ['SOPHOS_INSTALL']


PLUGIN_NAME = "av"


def av_plugin_dir():
    return os.path.join(sophos_spl_path(), 'plugins', PLUGIN_NAME)


def av_exec_path():
    return os.path.join(av_plugin_dir(), 'sbin', PLUGIN_NAME)


def sophos_threat_detector_exec_path():
    return os.path.join(av_plugin_dir(), 'sbin', "sophos_threat_detector")

def log_path():
    return os.path.join(av_plugin_dir(), 'log')

