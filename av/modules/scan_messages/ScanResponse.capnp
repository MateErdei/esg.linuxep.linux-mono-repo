# Copyright 2019-2023 Sophos Limited. All rights reserved.
@0xe07ab885c495921e;

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

struct FileScanResponse {
    struct Detection {
        filePath        @0 :Text;
        threatType      @1 :Text;
        threatName      @2 :Text;
        sha256          @3 :Text;
    }

    detections      @0 :List(Detection);
    errorMsg        @1 :Text;
}
