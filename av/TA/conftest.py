#!/bin/env python3
# -*- coding: utf-8 -*-
# Copyright (C) 2020 Sophos Ltd
# All rights reserved.

# Run with as: python3 -u -m pytest

import pytest

import sys
import os

sys.path.append('/opt/test/inputs/test_scripts')
sys.path.append('/opt/test/inputs/test_scripts/Libs')
os.environ['TA_DIR'] = os.path.abspath(os.path.dirname(__file__))
sys.path.append(os.environ['TA_DIR'])


from Libs.fixtures.BaseMockService import BaseMockService, install_component, component_test_setup
from Libs.fixtures.AVPlugin import AVPlugin
from Libs.fixtures.SophosThreatDetector import SophosThreatDetector
from _pytest.runner import runtestprotocol
from pathlib import Path
import os
import shutil


def pytest_configure():
    pytest.sophos_install_location = ""


@pytest.fixture(scope="session")
def install_component_setup(tmpdir_factory):
    """
    Required to be executed once during the test, it copies the files from the component SDDS to the 'SOPHOS_INSTALL'
    a temporary directory setup by pytest, usually inside /var/tmp/pytest-of-xxxx/ (where xxxx is the hostname)

    :param tmpdir_factory:
    :return:
    """
    dir_path = tmpdir_factory.mktemp("sophos-spl")
    sophos_install = dir_path.strpath
    install_component(sophos_install)
    pytest.sophos_install_location = sophos_install
    print("Current install location: {}".format(pytest.sophos_install_location))

    #clear previous test logs
    test_logs_dir = "/opt/test/logs/test_logs"

    # Clear old logs if running via debug-loop
    shutil.rmtree(test_logs_dir, ignore_errors=True)
    return sophos_install


@pytest.fixture(scope="class")
def sspl_mock(install_component_setup):
    sophos_install = install_component_setup
    component_test_setup(sophos_install)
    component = BaseMockService(sophos_install)
    yield component
    component.cleanup()


@pytest.fixture(scope="class")
def av_plugin_instance():
    av = AVPlugin()
    yield av
    av.stop_av()


@pytest.fixture(scope="class")
def sophos_threat_detector():
    av = SophosThreatDetector()
    yield av
    av.stop()


def collect_logs(test_name):
    """
    Copy all logs from the install directory
    """
    dir_to_export_logs_to = "/opt/test/logs/test_logs/{}".format(test_name)
    if os.path.exists(dir_to_export_logs_to):
        shutil.rmtree(dir_to_export_logs_to)
    os.makedirs(dir_to_export_logs_to)
    for filename in Path(pytest.sophos_install_location).rglob('*.log'):
        print("Copying log file: {} to {}".format(filename, dir_to_export_logs_to))
        shutil.copy(filename, dir_to_export_logs_to)


@pytest.hookimpl(tryfirst=True, hookwrapper=True)
def pytest_runtest_makereport(item, call):
    # execute all other hooks to obtain the report object
    outcome = yield
    report = outcome.get_result()

    if report.failed:
        print("Current install location: {}".format(pytest.sophos_install_location))
        print("Result: {}, Test: {}".format(item.name, report.outcome))
        collect_logs(item.name)
