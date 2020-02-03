Command line used to find this crash:

/home/pair/gesner/sspl-tools/afl-2.52b//afl-fuzz -i /home/pair/gesner/sspl-tools/everest-systemproducttests/SupportFiles/base_data/fuzz//parsealcpolicytests/ -o findings_parsealcpolicytests -d -m 200 /home/pair/gesner/sspl-tools/everest-base/cmake-afl-fuzz/tests/FuzzTests/parsealcpolicytests

If you can't reproduce a bug outside of afl-fuzz, be sure to set the same
memory limit. The limit used for this fuzzing session was 200 MB.

Need a tool to minimize test cases before investigating the crashes or sending
them to a vendor? Check out the afl-tmin that comes with the fuzzer!

Found any cool bugs in open-source tools using afl-fuzz? If yes, please drop
me a mail at <lcamtuf@coredump.cx> once the issues are fixed - I'd love to
add your finds to the gallery at:

  http://lcamtuf.coredump.cx/afl/

Thanks :-)
