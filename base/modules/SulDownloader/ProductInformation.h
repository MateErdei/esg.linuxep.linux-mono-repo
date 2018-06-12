//
// Created by pair on 05/06/18.
//

#ifndef SULDOWNLOADER_PRODUCTINFORMATION_H
#define SULDOWNLOADER_PRODUCTINFORMATION_H

#include <string>
#include <vector>

#include "Tag.h"

namespace SulDownloader
{

    class ProductInformation
    {
    public:
        const std::string& getLine() const;
        void setLine(const std::string& line);
        const std::string& getName() const;
        void setName(const std::string& name);
        void setTags(std::vector<Tag> tags);
        bool hasTag(const std::string & releaseTag) const ;

        std::string getBaseVersion() const;
        const std::string& getVersion() const;
        void setVersion(const std::string &version);

    private:
        std::vector<Tag> m_tags;
        std::string m_line;
        std::string m_name;
        std::string m_version;



    };

}
#endif //SULDOWNLOADER_PRODUCTINFORMATION_H
