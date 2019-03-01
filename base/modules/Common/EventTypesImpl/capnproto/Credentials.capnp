#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xd460b7bdbd3df54d;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct CredentialsEvent {
    enum EventType {
        authSuccess             @0;
        authFailure             @1;
        created                 @2;
        deleted                 @3;
        passwordChange          @4;
        membershipChange        @5;
    }

    enum SessionType {
        network                 @0;
        networkInteractive      @1;
        interactive             @2;
    }

    eventType                   @0 :EventType;
    sessionType                 @1 :SessionType;
    subjectUserSID              @2 :Common.UserSID;
    targetUserSID               @3 :Common.UserSID;
    timestamp                   @4 :Common.Timestamp;
    logonID                     @5 :UInt32;
    remoteNetworkAddress        @6 :Common.NetworkAddress;

    userGroupID                 @7 :UInt32;
    userGroupName               @8 :Text;
    sophosPid                   @9 :Common.SophosPID;

}
