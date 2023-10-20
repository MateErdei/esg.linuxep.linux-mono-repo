# Copyright 2023 Sophos Limited. All rights reserved.
load(":version_info.bzl", "AV_LINE_ID", "AV_PLUGIN_NAME")

SUBSTITUTIONS = {
    "@PLUGIN_NAME@": AV_PLUGIN_NAME,
    "@PRODUCT_LINE_ID@": AV_LINE_ID,
    "@AV_PRETTY_NAME@": "Sophos Linux AntiVirus",
}
