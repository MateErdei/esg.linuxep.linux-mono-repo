#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xf1006eb37381daee;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct ImageEvent {
    enum EventType {
        loaded                   @0;
    }

    eventType                    @0 :EventType;
    sophosPID                    @1 :Common.SophosPID;
    loadTime                     @2 :Common.Timestamp;
    imageBase                    @3 :UInt64;
    imageSize                    @4 :UInt32;
    fileSize                     @5 :Common.OptionalUInt64;
    pathname                     @6 :Common.Pathname;
    sha256                       @7 :Common.SHA256;
    sha1                         @8 :Common.SHA1;
    pesha256                     @9 :Common.SHA256;
    pesha1                      @10 :Common.SHA1;
}