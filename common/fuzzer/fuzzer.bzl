# Copyright 2024 Sophos Limited. All rights reserved.

load("//tools/config:soph_cc_rules.bzl", "soph_cc_binary")

#-fno-omit-frame-pointer provides nicer stack traces in error messages
#There are additional options to get 'perfect' stack traces  https://clang.llvm.org/docs/AddressSanitizer.html
FUZZ_FLAGS = struct(
    COPTS = [
        "-fsanitize=fuzzer,address,undefined",
        "-fsanitize-recover=address",
        "-fsanitize=nullability,integer,implicit-conversion,implicit-integer-arithmetic-value-change,implicit-integer-truncation",
        "-fno-omit-frame-pointer",
    ],
    LINKOPTS = [
        "-fsanitize=fuzzer,address,undefined",
        "-fsanitize-recover=address",
        "-fsanitize=nullability,integer,implicit-conversion,implicit-integer-arithmetic-value-change,implicit-integer-truncation",
        "-fno-omit-frame-pointer",
    ],
    DEFINES = [
        "USING_LIBFUZZER",
    ],
)

def soph_libfuzzer_cc_binary(
        name,
        visibility,
        **kwargs):
    """
    Compile a cc_binary (via soph_cc_binary) with libfuzzer specific attributes.

    Args:
        see soph_cc_binary in soph_cc_rules.bzl
    """
    flag_dict = {
        "copts": FUZZ_FLAGS.COPTS,
        "defines": FUZZ_FLAGS.DEFINES,
        "linkopts": FUZZ_FLAGS.LINKOPTS,
    }

    for attr, attr_contents in flag_dict.items():
        if attr in kwargs:
            kwargs[attr].append(attr_contents)
        else:
            kwargs[attr] = attr_contents

    soph_cc_binary(
        name = name,
        visibility = visibility,
        **kwargs
    )
