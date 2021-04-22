#------------------------------------------------------------------------------
# Copyright 2021 Sophos Limited. All rights reserved.
#
#------------------------------------------------------------------------------

@0xc9331743a30cc059;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

struct ProcessControl {
    commandType                      @0  :Int64;
}


