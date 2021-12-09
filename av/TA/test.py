def _get_log_contents(path_to_log):
    try:
        with open(path_to_log, "rb") as log:
            contents = log.read()
    except FileNotFoundError:
        return None
    except EnvironmentError:
        return None
    return contents.decode("UTF-8", errors='backslashreplace')

def mark_expected_error_in_log(log_location, error_message):
    error_string = "ERROR"
    mark_string = "expected-error"
    contents = _get_log_contents(log_location)
    if contents is None:
        print("File not found not marking expected error, if you are you error checking then the error won't exist!")
        return

    for line in contents.splitlines():
        if error_message in line:
            new_line = line.replace(error_string, mark_string)
            contents = contents.replace(line, new_line)

    with open(log_location, "w") as log:
        log.write(contents)


mark_expected_error_in_log("/opt/test/inputs/test_scripts/test_file", "as it is a Zip Bomb")