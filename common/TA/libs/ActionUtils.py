# Copyright 2023 Sophos Limited. All rights reserved.

import datetime
import time


def get_valid_creation_time_and_ttl():
    ttl = int(time.time())+10000
    creation_time = datetime.datetime.now().strftime("%Y-%m-%dT%H:%M:%S.%fZ")
    return f"{creation_time}_{ttl}"
