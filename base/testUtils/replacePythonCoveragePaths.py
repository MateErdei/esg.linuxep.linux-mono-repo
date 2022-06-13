import argparse
import sqlite3

# This script will replace substrings in file paths in a python coverage file.
# This is useful when paths need to be corrected in the generated coverage report

def add_options():
    parser = argparse.ArgumentParser(description='Replace substrings in file paths in a python coverage DB file.')
    parser.add_argument('-c', '--coverage-file', required=True, action='store', help="The coverage DB file to modify")
    parser.add_argument('-f', '--to-find', required=True, action='store', help="Substring to be found and replaced")
    parser.add_argument('-r', '--to-replace-with', required=True, action='store', help="String that will replace the found one")
    return parser

def main():
    parser = add_options()
    args = parser.parse_args()
    print("Running with:")
    print(f"Coverage file: {args.coverage_file}")
    print(f"Will replace: {args.to_find}")
    print(f"With: {args.to_replace_with}")

    sql_cmd = f"UPDATE file SET path = REPLACE(path, '{args.to_find}', '{args.to_replace_with}');"
    print(f"Will run: {sql_cmd}")
    conn = sqlite3.connect(args.coverage_file)
    cursor = conn.cursor()
    cursor.execute(sql_cmd)
    conn.commit()

if __name__ == '__main__':
    main()
