cp output/pluginapi.tar.gz /build/input/
cp output/cmcsrouterapi.tar.gz /build/input/
rm -rf /build/input/base-sdds
cp -r output/SDDS-COMPONENT /build/input/base-sdds

rm -rf ../esg.linuxep.sspl-warehouse/redist/base/latest/x64
cp -r --no-preserve=mode output/SDDS-COMPONENT ../esg.linuxep.sspl-warehouse/redist/base/latest/x64
rm -rf ../esg.linuxep.sspl-warehouse/redist/responseactions/latest/x64
cp -r --no-preserve=mode output/RA-SDDS3-PACKAGE ../esg.linuxep.sspl-warehouse/redist/responseactions/latest/x64
rm -f ../esg.linuxep.sspl-warehouse/SystemProductTestOutput.tar.gz
cp output/SystemProductTestOutput.tar.gz ../esg.linuxep.sspl-warehouse
