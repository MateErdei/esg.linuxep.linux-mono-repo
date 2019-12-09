def to_byte_string(s):
    if isinstance(s, str):
        return s.encode("UTF-8")
    else:
        return str(s)