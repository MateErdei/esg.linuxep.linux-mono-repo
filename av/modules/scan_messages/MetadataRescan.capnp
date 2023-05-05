# Copyright 2023 Sophos Limited. All rights reserved.
@0xcb50fc79135e87f7;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

struct MetadataRescan {
    threatType @0 :Text;
    threatName @1 :Text;
    filePath @2 :Text;
    sha256 @3 :Text;
}


