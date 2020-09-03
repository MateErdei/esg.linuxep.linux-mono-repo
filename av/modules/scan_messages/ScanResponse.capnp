#------------------------------------------------------------------------------
# Copyright 2019 Sophos Limited. All rights reserved.
#
#------------------------------------------------------------------------------

@0xfbd67d91fed68ded;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

## A FileScanRequest also has a file descriptor send as aux data
struct FileScanResponse {
    struct Detection {
        filePath        @0 :Text;
        threatName      @1 :Text;
    }

    detections      @0 :List(Detection);
    fullScanResult  @1 :Text; ## JSON response from SUSI
    errorMsg        @2 :Text;
}
