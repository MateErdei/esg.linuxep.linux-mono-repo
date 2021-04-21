#------------------------------------------------------------------------------
# Copyright 2020 Sophos Limited. All rights reserved.
#
#------------------------------------------------------------------------------
@0x913548128bca14cd;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

struct NamedScan {
    # The data required for a scan to run - passed on stdin
    name                @0 :Text;
    excludePaths        @1 :List(Text);
    scanHardDisc        @2 :Bool;
    scanNetwork         @3 :Bool;
    scanOptical         @4 :Bool;
    scanRemovable       @5 :Bool;
}
