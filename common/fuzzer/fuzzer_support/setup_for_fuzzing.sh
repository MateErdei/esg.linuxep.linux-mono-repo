#!/usr/bin/env bash
set -ex

SYMBOLIZER=/usr/lib/llvm-12/bin/llvm-symbolizer

useradd sophos-spl-user
#sanitizer instrumentation
sudo apt-get install -y llvm-12

[[ -f $SYMBOLIZER ]] || { echo "no symbolizer"; find /usr/ -name llvm-symbolizer; exit 1; }

export PATH=$SYMBOLIZER:$PATH

#Allows better reporting
export ASAN_OPTIONS=halt_on_error=0
export LSAN_OPTIONS=verbosity=1:log_threads=1
export UBSAN_OPTIONS=print_stacktrace=1
