#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xda98cdb0efaf7ff7;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct FilePropertyEvent {
    enum EventType {
        set                      @0;
        unset                    @1;
    }

    eventType                    @0 :EventType;
    sophosPID                    @1 :Common.SophosPID;
    sophosTID                    @2 :Common.SophosTID;
    propertyDataLength           @3 :UInt32;
    propertyId                   @4 :UInt32;
    createTime                   @5 :Common.Timestamp;
    sha256Hash                   @6 :Common.SHA256;
    propertyData                 @7 :Data;
}