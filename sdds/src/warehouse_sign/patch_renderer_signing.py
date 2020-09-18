"""
Patch the SDDS Renderer warehouse_sign script to use the new warehouse_sign tools

Out of the box, the Grinch template tries to connect to buildsign-m to sign warehouse content.
2.1 build machines don't have access to the old signing server and must use the new tools, so
replace the script with one which uses the new tools.
"""
import os

SIGN_SCRIPT_PATH = r'C:\Program Files (x86)\Sophos Plc\SDDS Renderer\warehouse_sign.py'


def main():
    print('Patching the renderer warehouse_sign script...')
    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
    print('Repo root:', repo_root)
    template_path = os.path.join(repo_root, 'src', 'warehouse_sign', 'warehouse_sign_template.py')
    print('Reading template:', template_path)

    with open(template_path) as f:
        template_content = f.read()

    print('Replacing script at:', SIGN_SCRIPT_PATH)
    # Supply a path to the new script
    template_content = template_content.replace('<BASE_PATH>', repo_root)
    # Supply the build environment - required to use the new warehouse_sign tools
    template_content = template_content.replace("{'<TEMPLATE': 'ENV>'}", str(os.environ.copy()))

    with open(SIGN_SCRIPT_PATH, 'w') as f:
        f.write(template_content)

    print('Patched.')


if __name__ == '__main__':
    main()
