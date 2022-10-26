## Copyright 2022, Sophos Limited.  All rights reserved.
@0xdb9c519af36fd294;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");


struct QuarantineResponseMessage {
    result      @0  :Int64;
}
