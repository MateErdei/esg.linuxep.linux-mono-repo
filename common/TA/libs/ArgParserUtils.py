#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Copyright (C) 2019 Sophos Plc, Oxford, England.
# All rights reserved.
import argparse
import os
import ssl

import PathManager

def add_common_test_args(parser):
    return NotImplementedError


def add_mcs_fuzz_tests_args(parser):
    parser.add_argument("--web-port", help="Web UI port", default=26001, type=int)
    parser.add_argument("--kitty-cmd-line", help="The command line to pass through to Kitty")
    parser.add_argument("--delay-between-tests", help="Delay (in seconds) between each test", default=1.0, type=float)
    parser.add_argument("--max-failures", help="maximum number of failures before test session is stopped", default=1,
                        type=int)
    parser.add_argument("--range",
                        help="define the ratio between the input mutation generated by fuzzer and the number of mutations that will be used in test session",
                        default=3,
                        type=int)
    parser.add_argument("--mutation-server-timeout", help="time server waits for fuzzer to generate a mutation",
                        default=10.0, type=float)

    parser.add_argument("--shutdown-web-ui", help="When the fuzz testing is done, close the web ui and exit",
                        action="store_true", default=False)

def add_mcsrouter_fuzz_tests_args(parser):
    parser.add_argument("--suite", default='mcs', type=str, \
                        help="Specify policy to fuzz", choices=['alc', 'mcs'])
    parser.add_argument("--using-python",
                        help="specify is test is ran using python './mcs_fuzz_test_runner.py' or via robot or not",
                        action="store_true", default=False)
    add_mcs_fuzz_tests_args(parser)

def tls_from_string(tls_string=""):
    tls_string_lower_case = tls_string.lower()
    if tls_string_lower_case == "tlsv1_2":
        return ssl.PROTOCOL_TLSv1_2
    elif tls_string_lower_case == "tlsv1_1":
        return ssl.PROTOCOL_TLSv1_1
    elif tls_string_lower_case == "tlsv1":
        return ssl.PROTOCOL_TLSv1
    elif tls_string_lower_case == "sslv3":
        return ssl.PROTOCOL_SSLv3
    elif tls_string_lower_case == "sslv2":
        return ssl.PROTOCOL_SSLv2
    elif tls_string_lower_case == "sslv23":
        return ssl.PROTOCOL_TLS
    else:
        return None

class store_ssl_tls(argparse.Action):
    def __call__(self, parser, namespace, tls_version_string, option_string=None):
        tls_version = tls_from_string(tls_version_string)
        setattr(namespace, self.dest, tls_version)

def add_cloudserver_args(parser):
    # 'Cloud test server for SSPL Linux automation'
    parser.add_argument("--reregister",
                        help="Force endpoint to re-register every time it sends status or requests commands",
                        default=False, action="store_true", dest="reregister")
    parser.add_argument("--daemon", help="Daemonise just before we start serving requests",
                        default=False, action="store_true", dest="daemon")
    parser.add_argument("--pidfile", help="Record the PID into a file",
                        default=None, action="store", dest="pidfile")
    parser.add_argument("--verifycookies", help="verify that the endpoint isn't sending us old cookies",
                        default=None, action="store_true", dest="verifycookies")
    parser.add_argument("--heartbeat-off", action="store_false", dest="heartbeat",
                        default=True,
                        help="Turn off heartbeat by default")
    parser.add_argument("--force-fail-registration", action="store_true", dest="failregistration",
                        default=False,
                        help="force all registration to fail")
    parser.add_argument("--force-fail-jwt", action="store_true", dest="failjwt",
                        default=False,
                        help="force authentication to fail")
    parser.add_argument("--force-break-jwt", action="store_true", dest="breakjwt",
                        default=False,
                        help="force authentication to be missing parts")
    parser.add_argument("--timeout", action="store_true", dest="timeout",
                        default=False,
                        help="force server to make connections hang")
    libs_dir = PathManager.get_libs_path()
    support_file_dir = PathManager.get_support_file_path()
    parser.add_argument("--port", help="define the port the server will listen to", default=4443, dest="port")
    parser.add_argument("--initial-alc-policy", help="define the initial alc policy used", default=os.path.join(support_file_dir, "CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_ALC_policy.xml"), dest="INITIAL_ALC_POLICY")
    parser.add_argument("--initial-mcs-policy", help="define the initial mcs policy used", default=os.path.join(support_file_dir, "CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_MCS_policy.xml"), dest="INITIAL_MCS_POLICY")
    parser.add_argument("--initial-sav-policy", help="define the initial sav policy used", default=os.path.join(support_file_dir, "CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_SAV_policy.xml"), dest="INITIAL_SAV_POLICY")
    parser.add_argument("--initial-core-policy", help="define the initial core policy used", default=os.path.join(support_file_dir, "CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_CORE_policy.xml"), dest="INITIAL_CORE_POLICY")
    parser.add_argument("--initial-corc-policy", help="define the initial corc policy used", default=os.path.join(support_file_dir, "CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_CORC_policy.xml"), dest="INITIAL_CORC_POLICY")
    parser.add_argument("--initial-livequery-policy", help="define the initial livequery policy used", default=os.path.join(support_file_dir, "CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_LiveQuery_policy.xml"), dest="INITIAL_LIVEQUERY_POLICY")
    parser.add_argument("--initial-flags", help="define the initial flags used", default=os.path.join(support_file_dir, "CentralXml/FakeCloudDefaultPolicies/FakeCloudDefault_FLAGS.json"), dest="INITIAL_FLAGS")
    parser.add_argument("--tls", dest="tls", default=ssl.PROTOCOL_SSLv23, action=store_ssl_tls, help="Set tls option", \
                        choices=["tlsv1", "tlsv1_1", "tlsv1_2", "sslv2", "sslv3", "sslv23"])