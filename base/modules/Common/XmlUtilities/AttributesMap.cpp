/******************************************************************************************************

Copyright 2018, Sophos Limited.  All rights reserved.

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
    class SimpleXmlParser
    {
    public:
        explicit SimpleXmlParser(size_t maxdepth = 10);

        /** need to be public as handlers will call them indirectly **/
        void onStartElement(const std::string& element, AttributePairCollection attributes, const std::string& id);
        void onEndElement(const std::string& element);
        void onTextHandler(const std::string& content);

        std::unordered_map<std::string, Attributes> attributesMap() const;
        std::vector<std::string> pathIds;

    private:
        std::string getElementPath(const std::string& currentElement, const std::string& id);

        std::stack<AttributesEntry> m_stack;
        std::unordered_map<std::string, Attributes> m_attributesMap;
        size_t m_maxdepth;
        std::string m_lastEntry;
        int m_entryCount;
    };

    /** general utility function **/

    std::pair<Common::XmlUtilities::AttributePairCollection, std::string> extractAttributes(const char** attr)
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

    static inline std::string rtrim(std::string s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(), [](char c) { return !std::isspace(c); }).base(), s.end());
        return s;
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

    /** implementation of SimpleXmlParser **/

    /** SimpleXmlParser **/
    SimpleXmlParser::SimpleXmlParser(size_t maxdepth) : m_stack(), m_attributesMap(), m_maxdepth(maxdepth), m_lastEntry(""), m_entryCount(0) {}

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
        m_attributesMap[topElement.fullpath] = Attributes(topElement.attributes);
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
            std::string number;
            //If ids are not unique add an integer to allow the map to disambiguate them.
            if (m_lastEntry != elementPath)
            {
                m_entryCount = 0;
            }
            else
            {
                std::stringstream appendNumber;
                appendNumber << "_" << m_entryCount++;
                number = appendNumber.str();
            }
            m_lastEntry = elementPath;
            elementPath = elementPath + number;
            pathIds.push_back(elementPath);
        }
        return elementPath;
    }

    std::unordered_map<std::string, Attributes> SimpleXmlParser::attributesMap() const { return m_attributesMap; };

    struct XMLParserHolder
    {
        XMLParserHolder() { parser = XML_ParserCreate(nullptr); }
        XML_Parser parser;
        ~XMLParserHolder() { XML_ParserFree(parser); }
    };

} // namespace

namespace Common
{
    namespace XmlUtilities
    {
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

        /** Attributes **/
        const std::string Attributes::TextId("TextId");
        Attributes::Attributes(AttributePairCollection attributes) : m_attributes(std::move(attributes)) {}

        Attributes::Attributes() : m_attributes() {}

        bool Attributes::empty() const { return m_attributes.empty(); }

        std::string Attributes::value(const std::string& attributeName, const std::string& defaultValue) const
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

        std::vector<std::string> AttributesMap::entitiesThatContainPath(const std::string& entityPath)
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

        /** parser **/
        AttributesMap parseXml(const std::string& xmlContent, int maxdepth)
        {
            SimpleXmlParser simpleXmlParser(maxdepth);

            XMLParserHolder parserHolder;

            XML_SetUserData(parserHolder.parser, &simpleXmlParser);

            XML_SetElementHandler(parserHolder.parser, startElement, endElement);

            XML_SetCharacterDataHandler(parserHolder.parser, textHandler);

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

            return AttributesMap(simpleXmlParser.attributesMap(), simpleXmlParser.pathIds);
        }

    } // namespace XmlUtilities
} // namespace Common