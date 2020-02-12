///////////////////////////////////////////////////////////
//
// Copyright (C) 2004 Sophos Plc, Oxford, England.
// All rights reserved.
//
//  StringSet.h
//  Implementation of the Class StringSet
//  $Revision: 4$ $Date: 17/08/2004 12:18:10$
///////////////////////////////////////////////////////////


#include <string>

#ifdef DEBUG
#include <ostream>
#endif /* DEBUG */

#include <vector>

typedef std::vector<std::string> StringSetBase;
/**
 * A Set of strings, implemented as an instantiation of an STL Vector object.
 */
class StringSet : public StringSetBase
{

public:
    /**
     * Determine if the provided string in the StringSet.
     * @return true if the argument is in the StringSet.
     *
     * @param testString    BORROWED reference to the string to test.
     */
    bool contains(const std::string& testString) const;

    /**
     * Returns a string containing all the strings held within the stringset
     * concatenated together with the provided separator.
     * @param concatenator    String to place between string elements.
     *
     */
    std::string concatenate(const std::string& concatenator) const;

    StringSet();

    virtual ~StringSet();

    StringSet(StringSet&& other) : StringSetBase(std::move(other))
        {}

        StringSet& operator=(StringSet&& other)
        {
            StringSetBase::operator=(std::move(other));
            return *this;
        }

    StringSet(const StringSet& other) : StringSetBase(other)
    {}

    StringSet& operator=(const StringSet& other)
    {
        StringSetBase::operator=(other);
        return *this;
    }

    /**
     * Constructor which creates a StringSet by breaking apart a string on a
     * character.
     * @param source    String to break apart.
     * @param separator    Character on which to split the source string.
     *
     */
    StringSet(const std::string& source, const char separator);

    /**
     * Creates a StringSet object with a single string entry.
     * @param value    string which is to be the sole contents of the StringSet.
     *
     */
    StringSet(const std::string& value);

    /**
     * Indicates whether the StringSet's contents are 'True'
     */
    bool isTrue() const;

    /**
     * Indicates whether the StringSet's contents are 'False'
     */
    bool isFalse() const;

    /**
     * Determines whether the parameter might be considered 'True'
     *
     * @param candidate    Value to test
     */
    static bool compareTrue(const std::string& candidate);
    /**
     * Determines whether the parameter might be considered 'False'
     *
     * @param candidate    Value to test
     */
    static bool compareFalse(const std::string& candidate);

    /**
     * Compare the contents of two StringSets where the order of their contents is
     * not important.
     * @return -1, 0 or 1 as for std::string::compare()
     *
     * @param candidate    StringSet for comparison
     */
    int compareUnordered(const StringSet& candidate) const;

    /**
     * Check if passed object contains at least all strings from this one.
     * @return true if candidate contains all strings from this object, false otherwise
     *
     * @param candidate    StringSet for comparison
     */
    bool isSubset(const StringSet& candidate) const;

    /**
     * Push an int onto the string set.
     */
    void push_back_int(const int value);

private:

    /**
     * Compare the contents of two StringSets
     * @return -1, 0 or 1,as per strcmp()
     *
     * @param candidate
     */
    int compare(const StringSet& candidate) const;

    /**
     * Determine whether the candidate string exists as one of the words in the
     * comma separated list.
     *
     * @param candidate    value to test
     * @param acceptableValues    Comma separated list of acceptable values
     */
    static bool compareBoolean(const std::string& candidate, const std::string& acceptableValues);

};
