#------------------------------------------------------------------------------
# Copyright 2020 Sophos Limited. All rights reserved.
#
#------------------------------------------------------------------------------

@0xf72baf069c614aa7;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

struct ThreatDetected {
    userID                          @0  :Text;
    detectionTime                   @1  :Text;
    threatType                      @2  :Text;
    scanType                        @3  :Text;
    notificationStatus              @4  :Text;
    threatID                        @5  :Text;
    idSource                        @6  :Text;
    fileName                        @7  :Text;
    filePath                        @8  :Text;
    actionCode                      @9  :Text;
}


