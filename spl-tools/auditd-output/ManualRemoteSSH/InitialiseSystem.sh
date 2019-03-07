#!/bin/bash

if [[ -f "/root/record_bin_events" ]]
then
    exit
fi

cat > /root/record_bin_events << EOF
#!/bin/bash
cat >> /root/AuditEvents.bin.tmp
EOF

chmod +x /root/record_bin_events

cat > /etc/audisp/plugins.d/record_bin_events << EOF
active = yes
direction = out
path = /root/record_bin_events
type = always
format = binary
EOF

setenforce Permissive &> /dev/null
sleep 5
service auditd restart
auditctl -D
