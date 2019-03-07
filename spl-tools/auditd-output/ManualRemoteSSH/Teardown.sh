#!/bin/bash

if [[ -f "/root/record_bin_events" ]]
then
    rm -f /root/record_bin_events
    rm -f /etc/audisp/plugins.d/record_bin_events
    service auditd restart
    setenforce Enforcing
fi
