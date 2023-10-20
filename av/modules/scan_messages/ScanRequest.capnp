# Copyright 2019-2023 Sophos Limited. All rights reserved.
@0xb0f44990900fd806;

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

## A FileScanRequest also has a file descriptor send as aux data
struct FileScanRequest {
    pathname                @0 :Text;
    scanInsideArchives      @1 :Bool;
    scanType                @2 :Int64;
    userID                  @3 :Text;
    scanInsideImages        @4 :Bool;
    pid                     @5 :Int64 = -1;
    executablePath          @6 :Text;
    detectPUAs              @7 :Bool;
    excludePUAs             @8 :List(Text);
}
