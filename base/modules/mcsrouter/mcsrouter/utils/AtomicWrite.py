#!/usr/bin/env python

import os
import logging
import codecs

def atomic_write(path, tmp_path, data):
    try:
        with codecs.open(tmp_path, mode="w", encoding='utf-8') as f:
            f.write(data.decode('utf-8'))
        os.rename(tmp_path, path)
    except (OSError, IOError) as e:
        logging.error("Atomic write failed with message: {}".format(e))

