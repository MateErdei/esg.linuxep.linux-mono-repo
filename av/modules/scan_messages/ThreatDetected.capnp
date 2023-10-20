# Copyright 2020-2023 Sophos Limited. All rights reserved.
@0xe7832c2ea3b7362e;

# For more information on what each field means check: LINUXDAR-1472

using Cxx = import "/capnp/c++.capnp";

$Cxx.namespace("Sophos::ssplav");

# A ThreatDetected also has a file descriptor sent over the socket using send_fd
struct ThreatDetected {
    userID                          @0  :Text;
    detectionTime                   @1  :Int64;
    threatType                      @2  :Text;
    threatName                      @3  :Text;
    scanType                        @4  :Int64;
    quarantineResult                @5  :Int64;
    filePath                        @6  :Text;
    sha256                          @7  :Text;
    threatId                        @8  :Text;
    isRemote                        @9  :Bool;
    reportSource                    @10 :Int64;
    pid                             @11 :Int64 = -1;
    executablePath                  @12 :Text;
    correlationId                   @13 :Text;
    threatSha256                    @14 :Text;
}


