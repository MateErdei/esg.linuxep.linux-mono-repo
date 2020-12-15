mkdir -p /opt/test/logs
mkdir -p /opt/test/results

echo "A" >/opt/test/logs/A
echo "B" >/opt/test/results/B

ls -lR /opt/test >/opt/test/logs/LS

