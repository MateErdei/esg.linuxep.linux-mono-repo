# Copyright 2021-2024 Sophos Limited. All rights reserved.

""" Create the documentation for tables in the Sophos osquery exetension """
import argparse
import json
import os
import re

DATATYPE_MAP = {
    "TEXT_TYPE": "text",
    "INTEGER_TYPE": "integer",
    "UNSIGNED_BIGINT_TYPE": "unsigned_bigint",  # UNSIGNED_BIGINT_TYPE first as this contains BIGINT_TYPE
    "BIGINT_TYPE": "bigint",
    "DOUBLE_TYPE": "double",
    "BLOB_TYPE": "blob",
}


def is_indexed(options):
    """Get if the column is indexed"""
    return "INDEX" in options


def is_hidden(options):
    """Get if the column is hidden"""
    return "HIDDEN" in options


def is_required(options):
    """Get if the column is required"""
    return "REQUIRED" in options


def get_type(type_param):
    """Get the osquery data type"""
    for key in DATATYPE_MAP:
        if key in type_param:
            return DATATYPE_MAP[key]
    raise RuntimeError("No valid osquery data type found")


def parse_comment(code, start_index):
    """Parse the comment strings in the header"""
    comment_end_index = code.rfind("*/", 0, start_index)
    between_comment_and_token = code[comment_end_index + 2 : start_index].strip()
    if between_comment_and_token != "":
        raise RuntimeError("Missing comment")

    comment_start_index = code.rfind("/*", 0, comment_end_index)
    comment = code[comment_start_index : comment_end_index + 2]
    comment_lines = comment.splitlines()
    if comment_lines[0].strip() != "/*":
        raise RuntimeError("Missing correct start comment format", comment_lines[0])
    if comment_lines[-1].strip() != "*/":
        raise RuntimeError("Missing correct end comment format", comment_lines[-1])

    comment_lines = comment_lines[1:-1]
    comments_dict = {}
    current_key = None
    for comment_line in comment_lines:
        stripped_line = comment_line.strip()
        if not stripped_line.startswith("*"):
            raise RuntimeError("Missing correct comment line format", stripped_line)
        trimmed_line = stripped_line[1:]
        if trimmed_line.startswith(" "):
            trimmed_line = trimmed_line[1:]
        if trimmed_line.startswith("@"):
            current_key = trimmed_line[1:]
            comments_dict[current_key] = []
        else:
            comments_dict[current_key].append(trimmed_line)

    return {k: "\n".join(v).rstrip() for k, v in comments_dict.items()}


def parse_table_docs(file_path):  # pylint: disable=too-many-statements
    """Parse the table macros in the header"""
    code = open(file_path, "r").read()

    table_matches = list(re.finditer(r"TABLE_PLUGIN_NAME\(([^)]*)\)", code))
    if not table_matches:
        print(f"Skipping: {file_path} as it does not contain TABLE_PLUGIN_NAME")
        return None
    if len(table_matches) > 1:
        raise RuntimeError(f"Error: Found more than one TABLE_PLUGIN_NAME in {file_path}")
    print(f"Parsing docs from {file_path}")

    table_match = table_matches[0]
    table_args = table_match[1].split(", ")
    table_comments = parse_comment(code, table_match.span()[0])

    table = {}
    table.update(table_comments)
    table["cacheable"] = False
    table["evented"] = False
    table["name"] = table_args[0]
    table["url"] = (
        "https://github.com/sophos-internal/esg.linuxep.linux-mono-repo/blob/develop/edr/modules/osqueryextensions/"
        + os.path.basename(file_path)
    )
    table["platforms"] = ["linux"]
    table["columns"] = []

    for column_match in re.finditer(r"TABLE_PLUGIN_COLUMN(?:_SNAKE_CASE)*\(([^)]*)\)", code):
        column_args = column_match[1].replace("\n", "").split(", ")
        column_comments = parse_comment(code, column_match.span()[0])
        if "description" not in column_comments:
            raise ValueError("column must have a description")
        if len(column_args) == 4:
            column = {}
            column.update(column_comments)
            column["description"] = "(DEPRECATED) " + column["description"]
            column["name"] = column_args[0].strip()
            column["type"] = get_type(column_args[2].strip())
            options = column_args[3].strip()
            column["index"] = is_indexed(options)
            column["hidden"] = is_hidden(options)
            column["required"] = is_required(options)
            table["columns"].append(column)
            column_snake_case = {}
            column_snake_case.update(column_comments)
            column_snake_case["name"] = column_args[1].strip()
            column_snake_case["type"] = get_type(column_args[2].strip())
            options = column_args[3].strip()
            column_snake_case["index"] = is_indexed(options)
            column_snake_case["hidden"] = is_hidden(options)
            column_snake_case["required"] = is_required(options)
            table["columns"].append(column_snake_case)
        else:
            column = {}
            column.update(column_comments)
            column["name"] = column_args[0].strip()
            column["type"] = get_type(column_args[1].strip())
            options = column_args[2].strip()
            column["index"] = is_indexed(options)
            column["hidden"] = is_hidden(options)
            column["required"] = is_required(options)
            table["columns"].append(column)

    return table


def main():
    """Main for document.py"""
    parser = argparse.ArgumentParser()
    parser.add_argument("--templates", nargs="+")
    parser.add_argument("--headers", nargs="+")
    parser.add_argument("--output-dir")
    args = parser.parse_args()

    tables = []
    for header_path in args.headers:
        table = parse_table_docs(header_path)
        if table:
            tables.append(table)

    os.makedirs(args.output_dir, exist_ok=True)
    with open(os.path.join(args.output_dir, "schema.json"), "w") as output_file:
        output_file.write(json.dumps(tables, indent=4))


if __name__ == "__main__":
    main()
