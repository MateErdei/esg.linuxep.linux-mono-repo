# Copyright 2023 Sophos Limited. All rights reserved.

load("//tools/config:soph_cc_rules.bzl", "soph_cc_test")

def spl_independent_cc_tests(name, srcs, per_test_srcs = [], **kwargs):
    """
    Adds soph_cc_test independently for each .cpp file in srcs

    This is to aid adding tests in SPL where currently tests often cannot run in parallel,
    due to a global FileSystem wrapper.
    """
    cpp_files = []
    h_files = []

    for file in srcs:
        if file.endswith(".cpp"):
            cpp_files.append(file)

    for file in srcs:
        if file.endswith(".h"):
            h_files.append(file)

    for cpp_file in cpp_files:
        soph_cc_test(
            name = name + "_" + cpp_file[:-4],
            srcs = [cpp_file] + h_files + per_test_srcs,
            **kwargs
        )
