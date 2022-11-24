# Copyright 2022 Sophos Limited. All rights reserved.
@0x9a5837436e1560b1;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

struct RestoreReport {
    time          @0 :Int64;
    path          @1 :Text;
    correlationId @2 :Text;
    wasSuccessful @3 :Bool;
}
