//
// Created by pair on 19/12/2019.
//

#ifndef SSPL_PLUGIN_MAV_FILEWALKER_H
#define SSPL_PLUGIN_MAV_FILEWALKER_H

#include "sophos_filesystem.h"

namespace fs = sophos_filesystem;

namespace filewalker
{
    class FileWalker
    {
    public:
        explicit FileWalker(fs::path p);

        void run();
    private:
        const fs::path m_starting_path;
    };
}

#endif //SSPL_PLUGIN_MAV_FILEWALKER_H
