# Copyright 2023 Sophos Limited. All rights reserved.
@0xcb50fc79135e87f7;

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

struct MetadataRescan {
    threatType @0 :Text;
    threatName @1 :Text;
    threatSha256 @2 :Text;
    filePath @3 :Text;
    sha256 @4 :Text;
}


