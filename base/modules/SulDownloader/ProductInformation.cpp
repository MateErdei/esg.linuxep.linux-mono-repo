//
// Created by pair on 05/06/18.
//

#include "ProductInformation.h"

namespace SulDownloader
{

    const std::string &ProductInformation::getName() const
    {
        return m_name;
    }

    void ProductInformation::setName(const std::string & name)
    {
        m_name = name;
    }

    void ProductInformation::setTags(std::vector<Tag> tags)
    {
        m_tags = tags;
    }

    void ProductInformation::setPHandle(SU_PHandle productHandle)
    {
        m_productHandle = productHandle;
    }

    SU_PHandle ProductInformation::getPHandle()
    {
        return m_productHandle;
    }

    bool ProductInformation::hasRecommended() const
    {
        for (auto &tag : m_tags)
        {
            if (tag.tag == "RECOMMENDED")
            {
                return true;
            }
        }
        return false;
    }

    const std::string &ProductInformation::getBaseVersion() const
    {
        return m_baseVersion;
    }

    void ProductInformation::setBaseVersion(const std::string &baseVersion)
    {
        m_baseVersion = baseVersion;
    }
}