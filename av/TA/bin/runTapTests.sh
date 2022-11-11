#! /bin/bash
set -e
[[ -d /vagrant ]]
(( UID == 0 )) || exec sudo -H "$0" "$@"
cd /vagrant/sspl-plugin-anti-virus/TA/
python3 -m robot -i "integration OR product" -e "ostia OR manual OR disabled OR stress" --randomize none --removekeywords wuks .
