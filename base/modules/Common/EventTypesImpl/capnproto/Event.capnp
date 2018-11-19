#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
@0xe2af1fdf777a2ce2;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

struct Event {
    union {
        #
        #   NOTE!!! New events must be added at the end and use the next available number
        #
        directory            @0: import "Directory.capnp".DirectoryEvent;
        driver               @1: import "Driver.capnp".DriverEvent;
        file                 @2: import "File.capnp".FileEvent;
        fileProperty         @3: import "FileProperty.capnp".FilePropertyEvent;
        image                @4: import "Image.capnp".ImageEvent;
        process              @5: import "Process.capnp".ProcessEvent;
        processProperty      @6: import "ProcessProperty.capnp".ProcessPropertyEvent;
        registry             @7: import "Registry.capnp".RegistryEvent;
        system               @8: import "System.capnp".SystemEvent;
        thread               @9: import "Thread.capnp".ThreadEvent;
        fileHash            @10: import "FileHash.capnp".FileHashEvent;
        dns                 @11: import "Dns.capnp".DnsEvent;
        http                @12: import "Http.capnp".HttpEvent;
        ip                  @13: import "Ip.capnp".IpEvent;
        network             @14: import "Network.capnp".NetworkEvent;
        url                 @15: import "Url.capnp".UrlEvent;
    }
}
