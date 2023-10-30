*** Settings ***
Documentation    Suite for Update Scheduler Plugin

Suite Teardown   Uninstall SSPL

Library    ${COMMON_TEST_LIBS}/FullInstallerUtils.py
