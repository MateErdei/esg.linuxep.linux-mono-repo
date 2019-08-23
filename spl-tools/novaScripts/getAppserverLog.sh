echo '---------- Hub Container Log: ----------'
docker logs hub_core --tail 500
echo ""

echo '---------- Hub Container Log Errors: ----------'
docker logs hub_core --tail 1000 |& grep -i error
echo ""

echo '---------- MCS Container Log: ----------'
docker logs lpas_mcs --tail 500
echo ""

echo '---------- MCS Container Log Errors: ----------'
docker logs lpas_mcs --tail 1000 |& grep -i error
echo ""

echo '---------- API Container Log: ----------'
docker logs api --tail 500
echo ""

echo '---------- API Container Log Errors: ----------'
docker logs api --tail 1000 |& grep -i error
echo ""

echo '---------- Nova Status ----------'
/home/ubuntu/g/nova/nova status

echo '---------- Startup Log ----------'
cat /tmp/cloudFormationInit.log

[[ -f "/home/ubuntu/using_stale_wars" ]] && printf "\nUsing Stale WAR Files! I.e aws s3 cp failed.\n"
