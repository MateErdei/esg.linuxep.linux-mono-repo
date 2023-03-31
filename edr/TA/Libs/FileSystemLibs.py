import os
import re

def does_file_exist(path):
    return os.path.isfile(path)

def does_file_contain_word(path, word) -> bool:
    with open(path) as f:
        pattern = re.compile(r'\b({0})\b'.format(word), flags=re.IGNORECASE)
        return bool(pattern.search(f.read()))
