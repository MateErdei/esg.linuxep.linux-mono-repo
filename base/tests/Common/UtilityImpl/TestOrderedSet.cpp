/******************************************************************************************************

Copyright 2019, Sophos Limited.  All rights reserved.

******************************************************************************************************/

#include <Common/UtilityImpl/OrderedSet.h>
#include <gtest/gtest.h>

using namespace Common::UtilityImpl;

TEST(OrderedSet, shouldKeepTheOrderAndReturnOnlyUniqueValues) // NOLINT
{
    std::vector<int> inputEntries = {1,1,2,1,3,2,3,2,1,4};
    OrderedSet<int> orderedSet;
    for( auto & e: inputEntries)
    {
        orderedSet.addElement(e);
    }
    std::vector<int> expected{1,2,3,4};
    EXPECT_EQ( orderedSet.orderedElements(), expected);
}
// demonstrating that user struct can be used as long as user defines operator == and specializes hash. 
struct ExampleOfUserProvidedStruct
{
    std::string a;
    std::string b;
};

// a class can use OrderedSet as long as defines an equality operator and a hash function as below
bool operator==(const ExampleOfUserProvidedStruct & lh, const ExampleOfUserProvidedStruct & rh)
{
    return lh.a == rh.a && lh.b == rh.b;
}
namespace std
{
    template<> struct hash<ExampleOfUserProvidedStruct>
    {
        typedef  ExampleOfUserProvidedStruct argument_type;
        typedef std::size_t result_type;
        result_type operator()(const argument_type& myStc) const noexcept
        {
            result_type const h1 ( std::hash<std::string>{}(myStc.a) );
            result_type const h2 ( std::hash<std::string>{}(myStc.b) );
            return h1 ^ h2;
        }
    };
}


TEST(OrderedSet, canBeAppliedToCustomStruct) // NOLINT
{

    std::vector<ExampleOfUserProvidedStruct> inputEntries = {
            {"a","b"},
            {"b","c"},
            {"a","b"}
    };
    OrderedSet<ExampleOfUserProvidedStruct> orderedSet;
    for( auto & e: inputEntries)
    {
        orderedSet.addElement(e);
    }
    std::vector<ExampleOfUserProvidedStruct> calculated = orderedSet.orderedElements();

    EXPECT_EQ(calculated.size(), 2);

}