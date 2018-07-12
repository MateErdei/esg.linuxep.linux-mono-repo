///////////////////////////////////////////////////////////
//
// Copyright (C) 2018 Sophos Plc, Oxford, England.
// All rights reserved.
//
///////////////////////////////////////////////////////////
#ifndef EVEREST_BASE_VERSIONEDCOPY_H
#define EVEREST_BASE_VERSIONEDCOPY_H

#include <Common/FileSystem/IFileSystem.h>

namespace Installer
{
    namespace VersionedCopy
    {
        class VersionedCopy
        {
        public:
            static int getDigitFromEnd(const std::string& s);
            static int versionedCopyMain(int argc, char* argv[]);
            static int versionedCopy(const Path& filename, const Path& DIST, const Path& INST);
        private:
            static bool same(const Path& file1, const Path& file2);
        };
    }
}

#endif //EVEREST_BASE_VERSIONEDCOPY_H
