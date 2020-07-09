#------------------------------------------------------------------------------
# Copyright 2019 Sophos Limited. All rights reserved.
#
#------------------------------------------------------------------------------
@0xc35ddb0378a7af1f;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

## A FileScanRequest also has a file descriptor send as aux data
struct FileScanRequest {
    pathname                @0 :Text;
    scanInsideArchives      @1 :Bool;
    scanType                @2 :Int64;
    userID                  @3 :Text;
}
