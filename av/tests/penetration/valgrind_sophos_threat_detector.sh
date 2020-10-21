#!/bin/bash

# Place this instead of /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector
# Assumes the real binary is at /opt/sophos-spl/plugins/av/sbin/sophos_threat_detector.0
# Assumes valgrind is installed

exec valgrind --log-file=/opt/sophos-spl/plugins/av/chroot/valgrind_sophos_threat_detector.log ${0%/*}/sophos_threat_detector.0 "$@"
