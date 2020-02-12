///////////////////////////////////////////////////////////
//
// Copyright (C) 2004 Sophos Plc, Oxford, England.
// All rights reserved.
//
//  StringSet.cpp
//  Implementation of the Class StringSet
//  $Revision: 6$ $Date: 19/08/2004 11:18:54$
///////////////////////////////////////////////////////////

#include "StringSet.h"

#include <algorithm>
#include <string>

const std::string GL_True_Values("1,true,enabled,enable,yes,on");
const std::string GL_False_Values("0,false,disabled,disable,no,off");


//LINKED-ATTRIBUTES
//LINKED-ATTRIBUTES-END

StringSet::StringSet()
{

}

/**
 * Constructor which creates a StringSet by breaking apart a string on a character.
 */
StringSet::StringSet(const std::string& source, const char separator)
{
    //std::string::size_type matchStart = 0;
    //std::string::size_type matchEnd;
    size_t matchStart = 0;
    size_t matchEnd;
    // find each element
    for(matchEnd = source.find(separator);
        matchEnd != std::string::npos;
        (matchStart = matchEnd + 1), (matchEnd = source.find(separator, matchStart)))
    {
        // matchStart points at the first character of the string which isn't the seperator
        // matchEnd points at the seperator
        if (matchEnd > matchStart)
        {
            // don't bother pushing empty strings
            push_back(source.substr(matchStart, matchEnd - matchStart));
        }
    }
    // and the final string too.
    push_back(source.substr(matchStart));
}


StringSet::~StringSet()
{
}

/**
 * Returns a new string containing all the strings held within the stringset
 * concatenated together with the provided separator.
 * @param concatenator    String to place between string elements.
 *
 */
std::string StringSet::concatenate(const std::string& concatenator) const
{
    const_iterator p;
    std::string result("");


    for(p = begin(); p != end(); ++p)
    {
        if (p != begin())
        {
            result.append(concatenator);
        }
        result.append(*p);
    }
    return result;
}

/**
 * Creates a StringSet object with a single string entry.
 */
StringSet::StringSet(const std::string& value)
{
    push_back(value);
}


/**
 * Determine if the provided string in the StringSet.
 * @return true if the argument is in the StringSet.
 */
bool StringSet::contains(const std::string& testString) const
{
    return std::find(begin(), end(), testString) != end();
}

bool StringSet::compareBoolean(const std::string& candidate, const std::string& acceptableValues)
{
    std::string value(candidate);


    if (candidate.find(",") != std::string::npos)
    {
        return false;
    }
    std::transform(value.begin(), value.end(), value.begin(), tolower);


    size_t start = acceptableValues.find(value);

    if (start == std::string::npos)
    {
        return false;
    }

    if (start == 0 || acceptableValues[start - 1] == ',' )
    {
        std::string::size_type end = start + value.size();
        std::string::size_type acceptableValuesSize = acceptableValues.size();
        std::string::value_type acceptableValueEndChar = acceptableValues[end];
        if (end >= acceptableValuesSize || acceptableValueEndChar == ',')
        {
            return true;
        }
    }

    return false;
}
bool StringSet::compareTrue(const std::string& candidate)
{
    return compareBoolean(candidate, GL_True_Values);
}

bool StringSet::compareFalse(const std::string& candidate)
{
    return compareBoolean(candidate, GL_False_Values);
}

bool StringSet::isTrue() const
{
    if (size() != 1)
    {
        return false;
    }

    return compareTrue(*begin());
}

bool StringSet::isFalse() const
{
    if (size() != 1)
    {
        return false;
    }

    return compareFalse(*begin());
}

int StringSet::compare(const StringSet& candidate) const
{
    int result = 0;

    if (size() < candidate.size())
    {
        return -1;
    }
    else if (size() > candidate.size())
    {
        return 1;
    }

    const_iterator cit = candidate.begin();
    // Note end condition of loop carefully: One of WW evil for loops
    for (const_iterator it = begin(); it != end() && result == 0; ++it, ++cit)
    {
        result = it->compare(*cit);
    }
    return result;
}

int StringSet::compareUnordered(const StringSet& candidate) const
{
    if (size() < candidate.size())
    {
        return -1;
    }
    else if (size() > candidate.size())
    {
        return 1;
    }
    for (const_iterator it = begin(); it != end(); ++it)
    {
        if (!candidate.contains(*it))
        {
            return 1;
        }
    }
    // nobody said a StringSet can't contain duplicate entries...
    for (const_iterator it = candidate.begin(); it != candidate.end(); ++it)
    {
        if (!contains(*it))
        {
            return -1;
        }
    }
    return 0;
}

bool StringSet::isSubset(const StringSet& candidate) const
{
    for (const_iterator it = begin(); it != end(); ++it)
    {
        if (!candidate.contains(*it))
        {
            return false;
        }
    }
    return true;
}

