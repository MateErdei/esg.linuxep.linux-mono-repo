#!/bin/sh
find / -xdev -type f -exec dd if={} of=/dev/null bs=1 count=1 status=none \;