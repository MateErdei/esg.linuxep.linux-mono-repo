# python3 does not define the values StringTypes, ListType or IntType that is used by
# katnip/legos/xml.py
# this is a HACK
from types import *
StringTypes=(str,bytes)
ListType=list
IntType=int
BooleanType=bool
DictionaryType=dict
