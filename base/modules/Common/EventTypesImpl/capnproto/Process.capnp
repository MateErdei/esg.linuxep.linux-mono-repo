#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xe53c848337d080f9;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct ProcessEvent {
    enum EventType {
        start                    @0;
        end                      @1;
    }

    eventType                    @0 :EventType;
    sophosPID                    @1 :Common.SophosPID;
    parentSophosPID              @2 :Common.SophosPID;
    parentSophosTID              @3 :Common.SophosTID;
    inheritSophosPID             @4 :Common.SophosPID;
    endTime                      @5 :Common.Timestamp;
    fileSize                     @6 :Common.OptionalUInt64;
    flags                        @7 :UInt32;
    sessionId                    @8 :UInt32;
    sid                          @9 :Common.SID;
    pathname                    @10 :Common.Pathname;
    cmdLine                     @11 :Text;
    sha256                      @12 :Common.SHA256;
    sha1                        @13 :Common.SHA1;
    pesha256                    @14 :Common.SHA256;
    pesha1                      @15 :Common.SHA1;
}