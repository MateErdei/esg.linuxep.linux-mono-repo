# Copyright 2023 Sophos Limited. All rights reserved.
load(":version_info.bzl", "RESPONSE_ACTIONS_LINE_ID", "RESPONSE_ACTIONS_PLUGIN_NAME")

SUBSTITUTIONS = {
    "@RA_PLUGIN_NAME@": RESPONSE_ACTIONS_PLUGIN_NAME,
    "@RA_PRODUCT_LINE_ID@": RESPONSE_ACTIONS_LINE_ID,
}
