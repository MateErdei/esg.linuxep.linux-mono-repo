# Copyright 2023-2024 Sophos Limited. All rights reserved.

load("//tools/config:soph_cc_rules.bzl", "soph_cc_test")

def spl_independent_cc_tests(name, srcs, per_test_srcs = [], **kwargs):
    """
    Adds soph_cc_test independently for each .cpp file in srcs

    This is to aid adding tests in SPL where currently tests often cannot run in parallel,
    due to a global FileSystem wrapper.
    """

    soph_cc_test(
        name = name,
        srcs = srcs + per_test_srcs,
        **kwargs
    )
