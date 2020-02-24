#------------------------------------------------------------------------------
# Copyright 2020 Sophos Limited. All rights reserved.
#
#------------------------------------------------------------------------------

@0xc889ef5d8e672583;

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

struct ThreatDetected {
    userID                          @0  :Text;
    detectionTime                   @1  :Int64;
    threatType                      @2  :Int64;
    threatName                      @3  :Text;
    scanType                        @4  :Int64;
    notificationStatus              @5  :Int64;
    filePath                        @6  :Text;
    actionCode                      @7  :Int64;
}


