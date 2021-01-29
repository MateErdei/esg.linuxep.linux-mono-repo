
import requests
import json

def extract_column_names_from_individual_table_info(table_info):
    return [ entry['name'] for entry in table_info['columns']]

def extract_tables_and_columns(schema):
    tables_and_columns = { entry['name']: extract_column_names_from_individual_table_info(entry) for entry in schema}
    return tables_and_columns

def get_all_names(tables_and_columns):
    name_set=set()
    for k,v in tables_and_columns.items():
        name_set.add(k)
        for col in v:
            name_set.add(col)
    return list(name_set)

def create_libfuzzer_dict(list_of_entries, destination):
    with open(destination, 'w') as file_handler:
        for entry in list_of_entries:
            file_handler.write('"{}"\n'.format(entry))

def load_json(url):
    r = requests.get(url)
    text = r.text
    schema = json.loads(text)
    return schema

def dict_from_osquery_schema( url, destination_path):
    schema = load_json(url)
    tables_and_columns = extract_tables_and_columns(schema)
    all_names = get_all_names(tables_and_columns)
    create_libfuzzer_dict(all_names, destination_path)


url = 'https://raw.githubusercontent.com/osquery/osquery-site/source/src/data/osquery_schema_versions/4.2.0.json'
destination_path = '/tmp/libfuzzer.dict'
dict_from_osquery_schema(url, destination_path)



