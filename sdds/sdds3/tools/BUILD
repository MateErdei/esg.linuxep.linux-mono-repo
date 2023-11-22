load("//tools/config:pylint.bzl", "pylint")
load("@pip_deps//:requirements.bzl", "requirement")

py_binary(
    name = "env",
    srcs = ["env.py"],
    python_version = "PY3",
    visibility = ["//sdds/sdds3:__pkg__"],
    deps = [
        requirement("pyyaml"),
    ],
)

filegroup(
    name = "maker",
    srcs = ["maker.py"],
    visibility = ["//sdds/sdds3:__pkg__"],
)
