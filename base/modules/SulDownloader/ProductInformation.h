//
// Created by pair on 05/06/18.
//

#ifndef SULDOWNLOADER_PRODUCTINFORMATION_H
#define SULDOWNLOADER_PRODUCTINFORMATION_H

#include <string>
#include <vector>

extern "C" {
#include <SUL.h>
}

#include "Tag.h"

namespace SulDownloader
{

    class ProductInformation
    {
    public:
        const std::string& getName() const;
        void setName(const std::string& name);
        void setTags(std::vector<Tag> tags);
        void setPHandle( SU_PHandle productHandle);
        SU_PHandle  getPHandle();
        //<PU_handle>
        //        tags,
        //        version,
        ///...
       bool hasRecommended() const ;

//    bool hasTag(string);
//
//    string name();
//
//    string fullversion();

    private:
        SU_PHandle m_productHandle;
        std::string m_name;
        std::string m_baseVersion;
    public:
        const std::string &getBaseVersion() const;

        void setBaseVersion(const std::string &baseVersion);

    private:
        std::vector<Tag> m_tags;



    };

}
#endif //SULDOWNLOADER_PRODUCTINFORMATION_H
