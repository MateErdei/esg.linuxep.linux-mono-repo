if [[ $USE_LIBFUZZER ]]
then
  cwd=$(pwd)
  rm $cwd/output/base-sdds/files/base/lib64/liblog4cplus-2.0.so.3
  rm $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libzmq.so.5
  rm $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libprotobuf.so.28
  rm $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/liblog4cplus-2.0.so.3

  ln -s $cwd/output/base-sdds/files/base/lib64/liblog4cplus-2.0.so.3.4.5 $cwd/output/base-sdds/files/base/lib64/liblog4cplus-2.0.so.3
  ln -s $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/liblog4cplus-2.0.so.3.4.5 $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/liblog4cplus-2.0.so.3
  ln -s $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libzmq.so.5.2.4 $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libzmq.so.5
  ln -s $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libprotobuf.so.28.0.3 $cwd/output/SDDS-COMPONENT/files/plugins/av/lib64/libprotobuf.so.28
fi