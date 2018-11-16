#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xd956312bb37f3ff5;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct RegistryEvent {
    enum EventType {
        keyCreated               @0;
        keyRenamed               @1;
        keyDeleted               @2;
        keyPermissionsModified   @3;
        keyOwnershipModified     @4;
        valueSet                 @5;
        valueDeleted             @6;
    }

    eventType                    @0 :EventType;
    sophosPID                    @1 :Common.SophosPID;
    sophosTID                    @2 :Common.SophosTID;
    keyName                      @3 :Text;
    sid                          @4 :Common.SID;

    valueName                    @5 :Text;
    valueType                    @6 :UInt32;
    valueSize                   @11 :UInt32;
    value                        @7 :Data; # This may be truncated if too big; valueSize is always correct.
    dacl                         @8 :Common.DACL;
    owner                        @9 :Common.SID;
    newKeyName                  @10 :Text;
}
