# Copyright 2020-2023 Sophos Limited. All rights reserved.

alias(
    name = "openssl",
    actual = select({
        "@platforms//os:windows": "@openssl_windows//:openssl",
        "@bazel_tools//src/conditions:linux_x86_64": "@openssl_linux-x64//:openssl",
        "@bazel_tools//src/conditions:linux_aarch64": "@openssl_linux-arm64//:openssl",
        "@platforms//os:macos": "@openssl_mac//:openssl",
    }),
    visibility = ["//visibility:public"],
)

alias(
    name = "libcrypto",
    actual = select({
        "@platforms//os:windows": "@openssl_windows//:libcrypto",
        "@bazel_tools//src/conditions:linux_x86_64": "@openssl_linux-x64//:libcrypto",
        "@bazel_tools//src/conditions:linux_aarch64": "@openssl_linux-arm64//:libcrypto",
        "@platforms//os:macos": "@openssl_mac//:libcrypto",
    }),
    visibility = ["//visibility:public"],
)

alias(
    name = "openssl_binary",
    actual = select({
        "@bazel_tools//src/conditions:linux_x86_64": "@openssl_linux-x64//:openssl_binary",
        "@bazel_tools//src/conditions:linux_aarch64": "@openssl_linux-arm64//:openssl_binary",
    }),
    visibility = ["//visibility:public"],
)
