#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xad8ddf4606543f23;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

using Common = import "Common.capnp";

struct NetworkEvent {
    enum EventType {
        tcpIPv4Connect           @0;
        tcpIPv4Accept            @1;
        tcpIPv4                  @2;
        udpIPv4                  @3;
        tcpIPv6Connect           @4;
        tcpIPv6Accept            @5;
        tcpIPv6                  @6;
        udpIPv6                  @7;
    }

    eventType                    @0 :EventType;
    sophosPID                    @1 :Common.SophosPID;
    sophosTID                    @2 :Common.SophosTID;
    startTime                    @3 :Common.Timestamp;
    sourceAddress                @4 :Common.SocketAddr;
    destinationAddress           @5 :Common.SocketAddr;
    dataSent                     @6 :UInt64;
    dataRecv                     @7 :UInt64;
}

# TCP connections will not have connect/accept set if the connection
# occurred before the data capture started.