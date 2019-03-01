/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#pragma once

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

namespace UpdateSchedulerImpl
{
    namespace configModule
    {
        inline boost::property_tree::ptree parseString(const std::string& input)
        {
            namespace pt = boost::property_tree;
            std::istringstream i(input);
            pt::ptree tree;
            pt::xml_parser::read_xml(i, tree, pt::xml_parser::trim_whitespace | pt::xml_parser::no_comments);
            return tree;
        }

        inline std::string toString(const boost::property_tree::ptree& tree)
        {
            namespace pt = boost::property_tree;
            std::ostringstream ost;
            pt::xml_parser::write_xml(ost, tree);
            return ost.str();
        }

    } // namespace configModule
} // namespace UpdateSchedulerImpl