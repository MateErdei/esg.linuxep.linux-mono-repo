#!/bin/bash

cwd=$(pwd)
rm -f $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libzmq.so.5
rm -f $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libprotobuf.so.28
rm -f $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/liblog4cplus-2.0.so.3
rm -f $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libstdc++.so.6

ln -s $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libstdc++.so.6.0.29 $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libstdc++.so.6
ln -s $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/liblog4cplus-2.0.so.3.4.6 $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/liblog4cplus-2.0.so.3
ln -s $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libzmq.so.5.2.4 $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libzmq.so.5
ln -s $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libprotobuf.so.28.0.3 $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libprotobuf.so.28
