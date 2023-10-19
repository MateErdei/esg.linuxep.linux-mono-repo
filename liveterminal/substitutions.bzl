# Copyright 2023 Sophos Limited. All rights reserved.
load(":version_info.bzl", "LIVE_RESPONSE_LINE_ID", "LIVE_RESPONSE_PLUGIN_NAME")

SUBSTITUTIONS = {
    "@LR_PLUGIN_NAME@": LIVE_RESPONSE_PLUGIN_NAME,
    "@LR_PRODUCT_LINE_ID@": LIVE_RESPONSE_LINE_ID,
}
