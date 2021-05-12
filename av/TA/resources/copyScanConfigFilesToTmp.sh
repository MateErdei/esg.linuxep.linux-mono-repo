#!/bin/bash

mkdir /tmp/config-files-test
chmod 777 /tmp/config-files-test
exec cp /opt/sophos-spl/plugins/av/var/*.config /tmp/config-files-test/
