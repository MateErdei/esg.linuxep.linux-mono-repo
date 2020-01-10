#------------------------------------------------------------------------------
# Copyright 2019 Sophos Limited. All rights reserved.
#
#------------------------------------------------------------------------------

@0xfbd67d91fed68ded;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

## A FileScanRequest also has a file descriptor send as aux data
struct FileScanResponse {
    clean           @0 :Bool;
    threatName      @1 :Text;
    fullScanResult  @2 :Text; ## JSON response from SUSI
}
