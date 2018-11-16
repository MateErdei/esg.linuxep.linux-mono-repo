#------------------------------------------------------------------------------
# Copyright 2018 Sophos Limited. All rights reserved.
#
# Sophos is a registered trademark of Sophos Limited and Sophos Group. All
# other product and company names mentioned are trademarks or registered
# trademarks of their respective owners.
#------------------------------------------------------------------------------
#
# Built-in Types https:#capnproto.org/language.html#built-in-types
#
# Void:               Types
# Boolean:            Bool
# Integers:           Int8, Int16, Int32, Int64
# Unsigned Integers:  UInt8, UInt16, UInt32, UInt64
# Floating-point:     Float32, Float64
# Blobs:              Text, Data
# Lists:              List(T)
#
# struct <struct-name> {
#     <field-name-0> @0 :<type>; # must start @0
#     ...
#     <field-name-n> @n-1 :<type>; # n-1th field
#     ...
# }
# 
# <union-name> :union {
#   <field-name-0> @0 :<type>; # must start @0
#   ...
#   <field-name-1> @n-1 :<type>; # n-1th field
#   ...
# }
#
@0xc95e525a140fd031;

using Cxx = import "capnp/c++.capnp";
$Cxx.namespace("Sophos::Journal");

# NOTE: 'Data' can be cast right back to a struct* type
using EventID = UInt32;
using FileAttributes = UInt32;
using GUID = Data;
using PID = UInt64;
using SID = Data;
using DACL = Data;
using SHA1 = Data;
using SHA256 = Data;
using TID = UInt64;
using Timestamp = UInt64;
using IpAddr = Data; # In network byte-ordering

struct FileId128 {
    sLowPart             @0 :UInt64;
    sHighPart            @1 :UInt64;
}

struct FileId {
    fileId               @0 :FileId128;
    volumeGUID           @1 :GUID;
}

struct FileInfo {
    numberOfHardLinks    @0 :UInt32;
    fileAttributes       @1 :FileAttributes;
}

struct FileTimestamps {
    creationTime         @0 :Timestamp;
    lastAccessTime       @1 :Timestamp;
    lastWriteTime        @2 :Timestamp;
    changeTime           @3 :Timestamp;
}

struct FileType {
    fileTypeClass        @0 :UInt32;
    fileType             @1 :UInt32;
}

struct SocketAddr {
    # Inet type can be inferred from the address size
    address                     @0 :IpAddr; # In network byte-ordering
    port                        @1 :UInt16;
}

struct IpFlow {
    sourceAddress       @0 :SocketAddr;
    destinationAddress  @1 :SocketAddr;
    protocol            @2 :UInt8;
}

struct Pathname {
    flags                @0 :UInt16;
    fileSystemType       @1 :UInt32;
    driveLetter          @2 :UInt8;
    pathname             @3 :Text;
    openName             @4 :TextOffsetLength;
    volumeName           @5 :TextOffsetLength;
    shareName            @6 :TextOffsetLength;
    extensionName        @7 :TextOffsetLength;
    streamName           @8 :TextOffsetLength;
    finalComponentName   @9 :TextOffsetLength;
    parentDirName       @10 :TextOffsetLength;
}

struct OptionalTimestamp { # Use this when when a Timestamp is immediately enclosed in a "optional :group" at the top level.
    value                @0 :Timestamp;
}

struct OptionalUInt8 { # Use this when when a UInt8 is immediately enclosed in a "optional :group" at the top level.
    value                @0 :UInt8;
}

struct OptionalUInt16 { # Use this when when a UInt16 is immediately enclosed in a "optional :group" at the top level.
    value                @0 :UInt16;
}

struct OptionalUInt32 { # Use this when when a UInt32 is immediately enclosed in a "optional :group" at the top level.
    value                @0 :UInt32;
}

struct OptionalUInt64 { # Use this when when a UInt64 is immediately enclosed in a "optional :group" at the top level.
    value                @0 :UInt64;
}

struct SophosPID { # Sophos Process ID
    osPID                @0 :PID; # Actual OS Process ID
    createTime           @1 :Timestamp;
}

struct SophosTID  { # Sophos Thread ID
    osTID                @0 :TID;     # Actual OS Thread ID
    createTime           @1 :Timestamp;
}

struct RedirectionInformation {

    # Redirection state flags. In theory both can be set
    redirectionState            @0 :UInt8;

    # Only used if redirectionState contains RedirectionStateInProgress
    originalDestination         @1 :SocketAddr;
    targetSophosPID             @2 :SophosPID; # May not be available

    # Only used if redirectionState contains RedirectionStateRedirected
    originalProcessPath         @3 :Pathname;
}

struct TextOffsetLength {
    length               @0 :UInt32; # Length in bytes
    offset               @1 :UInt32; # Offset in bytes
}

struct VersionInfo3 {
    major                @0 :UInt32;
    minor                @1 :UInt32;
    maintenance          @2 :UInt32;
}

struct VersionInfo4 {
    major                @0 :UInt32;
    minor                @1 :UInt32;
    maintenance          @2 :UInt32;
    build                @3 :UInt32;
}
