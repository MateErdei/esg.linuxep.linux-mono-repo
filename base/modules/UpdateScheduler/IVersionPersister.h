/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#pragma once

#include <algorithm>
#include <map>
#include <string>

// Line ID (rigid name)
typedef std::string LineId_t;

// Version of the rigid name
struct LineVersion {
    std::string version;
    std::string displayVersion;
};

// Case-insensitive comparison for Line IDs.
struct LineIdLessThan
{
    bool operator()(LineId_t const &lineId1, LineId_t const &lineId2) const
    {
        std::string lineId1_string = lineId1;
        std::string lineId2_string = lineId2;
        std::transform(lineId1_string.begin(), lineId1_string.end(), lineId1_string.begin(), ::tolower);
        std::transform(lineId2_string.begin(), lineId2_string.end(), lineId2_string.begin(), ::tolower);
        return 0 < lineId1_string.compare(lineId2_string);
    }
};

// Map of line IDs to their versions
typedef std::map<LineId_t, LineVersion, LineIdLessThan> LineVersionMap_t;

// Can be used to persist cloud subscription versions.
// The object to persist is a map that maps subscription rigid name (LineId_t)
// into the subscription fixed version (LineVersion_t).
// External (i.e. other than out-of-memory) errors are logged but not propagated.
struct IVersionPersister
{
    ~IVersionPersister() {}
    virtual LineVersionMap_t Load() = 0;
    virtual void Save(LineVersionMap_t const &) = 0;
};
