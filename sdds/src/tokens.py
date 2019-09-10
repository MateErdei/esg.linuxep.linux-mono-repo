"""
This is the tokens module for the warehouse definition maker.

It supplies one public function GetTokenDictionary() which
parses the supplied open xml file and extracts all the
tokens/token elements and uses them to create a dictionary.

One level of substitution is allowed, and tokens can be linked
to environment variables.
"""
import exceptions
import xml.etree.ElementTree as etree
import sys
import os
import logging
import re

class Value:
    def getValue(self):
        raise exceptions.NotImplementedError
    def makeSubstitutions(self, v):
        raise exceptions.NotImplementedError
class EnvironmentValue(Value):
    def __init__(self, name, vname):
        self._name=name
        self._vname=vname
    def getValue(self):
        try:
            return os.environ[self._vname]
        except KeyError:
            logging.error("In the configuration file '%s' requests a value from the environment variable '%s', but it was not found.", self._name, self._vname.upper())
            raise
    def makeSubstitutions(self, v):
        pass
class LiteralValue(Value):
    def __init__(self, value):
        self._value=value
    def getValue(self):
        return self._value
    def makeSubstitutions(self, v):
        self._value = self._value%v
class Token:
    def __init__(self, etree):
        self.name=etree.find("name").text
        logging.debug("Making token %s", self.name)
        v = etree.find("value")
        if v != None:
            logging.debug("adding literal value")
            self._value=LiteralValue(v.text)
        else:
            logging.debug("adding environment value")
            self._value=EnvironmentValue(self.name, etree.find("env").text)
    def getSubstitutionDictionary(self):
        return {self.name:self._value.getValue()}
    def makeSubstitutions(self, v):
        self._value.makeSubstitutions(v)
    
def GetTokenDictionary(configFile, substitutions):
    """
    This is the public function that analyses the config xml file and extracts
    elements from the data/tokens section.
    
    >>> GetTokenDictionary(file("test1.xml"), {})
    {'test': '$P$G-1.0.0.1000', 'harriet': '$P$G', 'dick': '1.0.0.1000', 'tom': 'fred'}

    >>> GetTokenDictionary(file("test2.xml"), {})
    Traceback (most recent call last):
        ...
    KeyError: 'harriet'
    """
    tree = etree.ElementTree(None, configFile)
    tokenElements = tree.findall("*/token")
    tokens = [Token(x) for x in tokenElements]

    # get the raw value for each token
    # any KeyErrors that are raised are ignored so that an
    # error is logged for each missing environment variable
    tokenDict = substitutions
    for x in tokens:
        try:
            logging.debug("getting substitutions for %s", x.name)
            tokenDict.update(x.getSubstitutionDictionary())
        except KeyError:
            pass

    # Tokens can include substitution of other tokens.
    # However, we only do one substitution pass, so there
    # should only be one level of substitution.
    #
    # KeyErrors raised here will not be caught and the script
    # will bomb out.
    for x in tokens:
        logging.debug("resolving substitutions for %s", x.name)
        x.makeSubstitutions(tokenDict)

    # get the substituted value for each token
    tokenDict = substitutions
    for x in tokens:
        logging.debug("getting substitutions for %s", x.name)
        tokenDict.update(x.getSubstitutionDictionary())

    return tokenDict        


def _test():
    import doctest
    doctest.testmod()

if __name__ == "__main__":
    logging.basicConfig()    
    _test()