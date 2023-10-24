# Copyright 2023 Sophos Limited. All rights reserved.
load(":version_info.bzl", "EDR_LINE_ID", "EDR_PLUGIN_NAME", "EDR_BASE_VERSION")

SUBSTITUTIONS = {
    "@PLUGIN_NAME@": EDR_PLUGIN_NAME,
    "@PRODUCT_LINE_ID@": EDR_LINE_ID,
    "@PLUGIN_VERSION@": EDR_BASE_VERSION,
}