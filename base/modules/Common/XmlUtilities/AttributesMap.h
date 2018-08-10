/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <string>
#include <vector>
#include <stack>
#include <unordered_map>

namespace Common
{
    namespace XmlUtilities
    {

        using AttributePair = std::pair<std::string, std::string>;
        using AttributePairCollection = std::vector<AttributePair>;


        class XmlUtilitiesException : public std::runtime_error
        {
        public:
            using std::runtime_error::runtime_error;
        };


        class Attributes
        {
        public:
            static const std::string TextId;
            explicit Attributes(  AttributePairCollection attributes);
            explicit Attributes();
            bool empty() const;
            std::string value( const std::string & attributeName, const std::string & defaultValue = "") const;

        private:
            AttributePairCollection m_attributes;
        };


        struct AttributesEntry
        {
            std::string fullpath;
            AttributePairCollection attributes;
            std::string content;
            void appendText(const std::string & );
        };


        class AttributesMap
        {
        public:
            explicit AttributesMap( std::unordered_map<std::string, Attributes> attributesMap, std::vector<std::string> idOrderedFullName );
            Attributes lookup( const std::string & entityFullPath) const;
            std::vector<std::string> entitiesThatContainPath(const std::string & entityPath);

        private:
            std::unordered_map<std::string, Attributes> m_attributesMap;
            std::vector<std::string> m_idOrderedFullName;
        };



        AttributesMap parseXml( const std::string & xmlContent, int maxdepth = 10);

    }
}


