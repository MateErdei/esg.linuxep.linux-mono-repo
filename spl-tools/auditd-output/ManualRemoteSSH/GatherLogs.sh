#!/bin/bash
cp /root/AuditEvents.bin.tmp ${1}AuditEvents.bin
cat /var/log/audit/audit.log > ${1}AuditEvents.log
ausearch -i -if ${1}AuditEvents.log > ${1}AuditEventsReport.log
