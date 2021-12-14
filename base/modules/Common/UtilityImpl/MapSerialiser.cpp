/******************************************************************************************************

Copyright 2021, Sophos Limited.  All rights reserved.

******************************************************************************************************/
#include "../thirdparty/nlohmann-json/json.hpp"
#include <map>

namespace Common::UtilityImpl
{
    void serialiseMap(std::map<std::string, std::string> map)
    {
        // create a map
//        std::map<std::string, int> m1 {{"key", 1}};

        // create and print a JSON from the map
        nlohmann::json j = map;
//        std::cout << j << std::endl;

        // get the map out of JSON
        std::map<std::string, int> m2 = j;

        // make sure the roundtrip succeeds
//        assert(m1 == m2);
    }
}