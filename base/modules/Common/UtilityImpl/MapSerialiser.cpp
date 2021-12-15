///******************************************************************************************************
//
//Copyright 2021, Sophos Limited.  All rights reserved.
//
//******************************************************************************************************/
//#include "../thirdparty/nlohmann-json/json.hpp"
//#include <map>
//
//namespace Common::UtilityImpl
//{
//
//    template <typename T>
//    std::string serialiseMap(std::map<std::string, T> map)
//    {
//        nlohmann::json j = map;
//        return j.dump();
//    }
//
////    std::string serialiseMap(std::map<std::string, std::string> map)
////    {
////        nlohmann::json j = map;
////        return j.dump();
////    }
//
//    std::map<std::string, std::string> deserialiseMap(std::string mapJson)
//    {
//        nlohmann::json jsonObj = nlohmann::json::parse(mapJson);
//        std::map<std::string, std::string> map = jsonObj;
//        return map;
//    }
//}