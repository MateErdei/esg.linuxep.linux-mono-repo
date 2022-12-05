# Copyright 2020-2022 Sophos Limited. All rights reserved.
@0xdb58d64963179a7d;

# For more information on what each field means check: LINUXDAR-1472

using Cxx = import "capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

# A ThreatDetected also has a file descriptor sent over the socket using send_fd
struct ThreatDetected {
    userID                          @0  :Text;
    detectionTime                   @1  :Int64;
    threatType                      @2  :Int64;
    threatName                      @3  :Text;
    scanType                        @4  :Int64;
    notificationStatus              @5  :Int64;
    filePath                        @6  :Text;
    actionCode                      @7  :Int64;
    sha256                          @8  :Text;
    threatId                        @9  :Text;
    isRemote                        @10 :Bool;
    reportSource                    @11 :Int64;
    pid                             @12 :Int64 = -1;
    executablePath                  @13 :Text;
}


