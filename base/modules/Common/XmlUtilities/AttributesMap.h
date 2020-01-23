/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace Common::XmlUtilities
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
        explicit Attributes(AttributePairCollection attributes, std::string contents);
        explicit Attributes();
        [[nodiscard]] auto empty() const -> bool;
        [[nodiscard]] auto value(const std::string& attributeName, const std::string& defaultValue = "") const -> std::string;
        [[nodiscard]] auto contents() const -> std::string;

    private:
        AttributePairCollection m_attributes;
        std::string m_contents;
    };

    class AttributesMap
    {
    public:
        explicit AttributesMap(
            std::unordered_map<std::string, Attributes> attributesMap,
            std::vector<std::string> idOrderedFullName);
        auto lookup(const std::string& entityFullPath) const -> Attributes;
        auto entitiesThatContainPath(const std::string& entityPath) const -> std::vector<std::string>;

        /**
         * Get all elements that start with the provided path.
         * Handles duplicate elements with or without ID.
         * @return Copied Attributes objects for every element that matches entityFullPath
         */
        auto lookupMultiple(const std::string& entityFullPath) const -> std::vector<Attributes>;

    private:
        std::unordered_map<std::string, Attributes> m_attributesMap;
        std::vector<std::string> m_idOrderedFullName;
    };

    const int DEFAULT_MAX_DEPTH = 10;

    /**
     * Parse an XML document, returning an AttributesMap
     * @param xmlContent
     * @param maxdepth Restrict entities to a max depth to prevent DOS.
     * @return AttributeMap
     */
    auto parseXml(const std::string& xmlContent, int maxdepth = DEFAULT_MAX_DEPTH) -> AttributesMap;

} // namespace Common::XmlUtilities
