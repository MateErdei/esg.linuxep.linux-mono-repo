#------------------------------------------------------------------------------
# Copyright 2020 Sophos Limited. All rights reserved.
#
#------------------------------------------------------------------------------
@0x8bf3416dfec66ed9;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

struct NamedScan {
    # The data required for a scan to run - passed on stdin
    name                @0 :Text;
    excludePaths        @1 :List(Text);
}
