//
// Created by pair on 06/06/18.
//

#ifndef EVEREST_TAG_H
#define EVEREST_TAG_H

#include <string>

namespace SulDownloader
{
    struct Tag
    {
        Tag(const std::string &t, const std::string &b, const std::string &l)
                : tag(t), baseversion(b), label(l)
        {
        }

        std::string tag;
        std::string baseversion;
        std::string label;
    };
}

#endif //EVEREST_TAG_H
