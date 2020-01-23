/******************************************************************************************************

Copyright 2018-2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "AttributesMap.h"

#include "Logger.h"

#include <algorithm>
#include <cassert>
#include <climits>
#include <expat.h>
#include <sstream>

namespace
{
    using namespace Common::XmlUtilities;

    struct AttributesEntry
    {
        std::string fullpath; // NOLINT(misc-non-private-member-variables-in-classes)
        AttributePairCollection attributes; // NOLINT(misc-non-private-member-variables-in-classes)
        std::string content; // NOLINT(misc-non-private-member-variables-in-classes)
        void appendText(const std::string&);
    };

    /** AttributesEntry **/
    void AttributesEntry::appendText(const std::string& appendContent)
    {
        if (content.empty())
        {
            content = appendContent;
        }
        else
        {
            content += "\n";
            content += appendContent;
        }
    }

    /** Stand-alone functions */

    inline auto rtrim(std::string s) -> std::string
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](char c) { return !std::isspace(c); }).base(), s.end());
        return s;
    }

    /** general utility function **/

    auto extractAttributes(const char** attr) -> std::pair<Common::XmlUtilities::AttributePairCollection, std::string>
    {
        std::string idValue;
        Common::XmlUtilities::AttributePairCollection attributesPair;
        for (int i = 0; attr[i]; i += 2)
        {
            std::string attributeName = attr[i];
            std::string attributeValue = attr[i + 1];
            if (attributeName == "id" || attributeName == "Id")
            {
                idValue = attributeValue;
            }
            attributesPair.emplace_back(AttributePair{ attributeName, attributeValue });
        }
        return { attributesPair, idValue };
    }

    struct XMLParserHolder
    {
        XMLParserHolder() { parser = XML_ParserCreate(nullptr); }
        XML_Parser parser; // NOLINT(misc-non-private-member-variables-in-classes)
        ~XMLParserHolder() { XML_ParserFree(parser); }
    };

    class SimpleXmlParser
    {
    public:
        explicit SimpleXmlParser(XML_Parser parser, size_t maxdepth = DEFAULT_MAX_DEPTH);

        /** need to be public as handlers will call them indirectly **/
        void onStartElement(const std::string& element, AttributePairCollection attributes, const std::string& id);
        void onEndElement(const std::string& element);
        void onTextHandler(const std::string& content);

        auto attributesMap() const -> std::unordered_map<std::string, Attributes>;

        std::vector<std::string> m_pathIds; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
        XML_Parser m_parser; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)

    private:
        auto getElementPath(const std::string& currentElement, const std::string& id) -> std::string;

        std::stack<AttributesEntry> m_stack;
        std::unordered_map<std::string, Attributes> m_attributesMap;
        size_t m_maxdepth;
        std::string m_lastEntry;
        int m_entryCount;
    };

    /** implementation of SimpleXmlParser **/
    SimpleXmlParser::SimpleXmlParser(XML_Parser parser, size_t maxdepth) :
        m_parser(parser),
        m_stack(),
        m_attributesMap(),
        m_maxdepth(maxdepth),
        m_lastEntry(""),
        m_entryCount(0)
    {
    }

    void SimpleXmlParser::onStartElement(
        const std::string& element,
        AttributePairCollection attributes,
        const std::string& id)
    {
        std::string elementPath = getElementPath(element, id);
        if (m_stack.size() >= m_maxdepth)
        {
            throw XmlUtilitiesException("Stack overflow. Max depth for the xml exceeded. ");
        }
        m_stack.push(AttributesEntry{ elementPath, std::move(attributes), "" });
    }

    void SimpleXmlParser::onEndElement(const std::string& element)
    {
        (void)element;
        auto& topElement = m_stack.top();
        assert(topElement.fullpath.find(element) != std::string::npos);
        if (!topElement.content.empty())
        {
            topElement.attributes.push_back(AttributePair{ Attributes::TextId, topElement.content });
        }
        m_attributesMap[topElement.fullpath] = Attributes(std::move(topElement.attributes), std::move(topElement.content));
        m_stack.pop();
    }

    void SimpleXmlParser::onTextHandler(const std::string& content)
    {
        std::string strippedContent = rtrim(content);
        if (!strippedContent.empty() && !m_stack.empty())
        {
            auto& topElement = m_stack.top();
            topElement.appendText(strippedContent);
        }
    }

    std::string SimpleXmlParser::getElementPath(const std::string& currentElement, const std::string& id)
    {
        std::string elementPath = m_stack.empty() ? currentElement : m_stack.top().fullpath + "/" + currentElement;
        if (!id.empty())
        {
            elementPath = elementPath + "#" + id;
        }
        // If ids are not unique add an integer to allow the map to disambiguate them.
        if (m_lastEntry != elementPath)
        {
            m_lastEntry = elementPath;
            m_entryCount = 0;
        }
        else
        {
            std::stringstream appendNumber;
            appendNumber << "_" << m_entryCount++;
            std::string number = appendNumber.str();
            elementPath = elementPath + number;
        }
        m_pathIds.push_back(elementPath); // A list of all element paths we use
        return elementPath;
    }

    std::unordered_map<std::string, Attributes> SimpleXmlParser::attributesMap() const
    {
        return m_attributesMap;
    }

    /** call backs for lib expat **/

    void startElement(void* data, const char* el, const char** attr)
    {
        using Common::XmlUtilities::AttributePairCollection;
        auto parser = static_cast<SimpleXmlParser*>(data);
        AttributePairCollection attributes;
        std::string idValue;
        std::tie(attributes, idValue) = extractAttributes(attr);
        parser->onStartElement(el, attributes, idValue);
    }

    void endElement(void* data, const char* el)
    {
        auto parser = static_cast<SimpleXmlParser*>(data);
        parser->onEndElement(el);
    }

    void textHandler(void* data, const char* text, int len)
    {
        std::string textstring(text, text + len);
        auto parser = static_cast<SimpleXmlParser*>(data);
        parser->onTextHandler(textstring);
    }

    void EntityDeclHandler(
        void* userData,
        const XML_Char* entityName,
        int /* is_parameter_entity */,
        const XML_Char* /*value*/,
        int /*value_length*/,
        const XML_Char* /*base*/,
        const XML_Char* /*systemId*/,
        const XML_Char* /*publicId*/,
        const XML_Char* /*notationName*/)
    {
        LOGERROR("Aborting XML parse due to entity " << entityName);
        auto parser = static_cast<SimpleXmlParser*>(userData);
        XML_StopParser(parser->m_parser, XML_FALSE);
    }

} // namespace

namespace Common::XmlUtilities
{
    /** Attributes **/
    const std::string Attributes::TextId("TextId"); // NOLINT(cert-err58-cpp)
    Attributes::Attributes(AttributePairCollection attributes, std::string contents)
        : m_attributes(std::move(attributes)),
          m_contents(std::move(contents))
    {}

    Attributes::Attributes() = default;

    auto Attributes::empty() const -> bool
    {
        return m_attributes.empty();
    }

    auto Attributes::value(const std::string& attributeName, const std::string& defaultValue) const -> std::string
    {
        for (auto& attribute : m_attributes)
        {
            if (attribute.first == attributeName)
            {
                return attribute.second;
            }
        }
        return defaultValue;
    }

    auto Attributes::contents() const -> std::string
    {
        return m_contents;
    }

    /** SimpleXml **/
    AttributesMap::AttributesMap(
        std::unordered_map<std::string, Attributes> attributesMap,
        std::vector<std::string> idOrderedFullName) :
        m_attributesMap(std::move(attributesMap)),
        m_idOrderedFullName(std::move(idOrderedFullName))
    {
    }

    Attributes AttributesMap::lookup(const std::string& entityFullPath) const
    {
        auto found = m_attributesMap.find(entityFullPath);
        if (found == m_attributesMap.end())
        {
            return Attributes();
        }
        return found->second;
    }

    std::vector<std::string> AttributesMap::entitiesThatContainPath(const std::string& entityPath) const
    {
        std::vector<std::string> fullPaths;
        for (const auto& fullPathEntry : m_idOrderedFullName)
        {
            if (fullPathEntry.find(entityPath) != std::string::npos)
            {
                fullPaths.push_back(fullPathEntry);
            }
        }
        return fullPaths;
    }

    std::vector<Attributes> AttributesMap::lookupMultiple(const std::string& entityFullPath) const
    {
        std::vector<Attributes> results;
        for (const auto& fullPathEntry : m_idOrderedFullName)
        {
            if (fullPathEntry.find(entityFullPath) == 0)
            {
                results.push_back(m_attributesMap.at(fullPathEntry));
            }
        }
        return results;
    }

    /** parser **/
    AttributesMap parseXml(const std::string& xmlContent, int maxdepth)
    {
        XMLParserHolder parserHolder;

        SimpleXmlParser simpleXmlParser(parserHolder.parser, maxdepth);

        XML_SetUserData(parserHolder.parser, &simpleXmlParser);

        XML_SetElementHandler(parserHolder.parser, startElement, endElement);

        XML_SetCharacterDataHandler(parserHolder.parser, textHandler);

        XML_SetEntityDeclHandler(parserHolder.parser, EntityDeclHandler);

        assert(xmlContent.size() < INT_MAX);
        if (XML_Parse(parserHolder.parser, xmlContent.data(), static_cast<int>(xmlContent.size()), true) ==
            XML_STATUS_ERROR)
        {
            std::stringstream errorInfoStream;
            errorInfoStream << "Error parsing xml: ";
            errorInfoStream << XML_ErrorString(XML_GetErrorCode(parserHolder.parser)) << "\n";
            errorInfoStream << "XmlLine: " << XML_GetCurrentLineNumber(parserHolder.parser);
            throw XmlUtilitiesException(errorInfoStream.str());
        }

        return AttributesMap(
                simpleXmlParser.attributesMap(),
                std::move(simpleXmlParser.m_pathIds)
                );
    }

} // namespace Common::XmlUtilities
