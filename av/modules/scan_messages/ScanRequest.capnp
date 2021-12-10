#------------------------------------------------------------------------------
# Copyright 2019 Sophos Limited. All rights reserved.
#
#------------------------------------------------------------------------------
@0xaa58c62c6d7f7805;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

## A FileScanRequest also has a file descriptor send as aux data
struct FileScanRequest {
    pathname                @0 :Text;
    scanInsideArchives      @1 :Bool;
    scanInsideImages        @2 :Bool;
    scanType                @3 :Int64;
    userID                  @4 :Text;
}
