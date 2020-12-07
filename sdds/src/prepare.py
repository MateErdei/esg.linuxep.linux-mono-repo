import os
import os.path
import locations
import winreg

def make_empty_folder(folder):
    if not folder == '' and not os.path.exists(folder):
        os.makedirs(folder)

def set_renderer_timeout(milliseconds):
    for view in (winreg.KEY_WOW64_64KEY, winreg.KEY_WOW64_32KEY):
        access = view | winreg.KEY_ALL_ACCESS
        with winreg.CreateKeyEx(winreg.HKEY_LOCAL_MACHINE, r"Software\Sophos", access=access) as key:
            winreg.SetValueEx(key, "RendererSigningTimeout", None, winreg.REG_DWORD, milliseconds)

if __name__ == '__main__':
    make_empty_folder(locations.LOGS)
    make_empty_folder(locations.WAREHOUSE_WRITE)
    make_empty_folder(locations.CUSTOMER_FILES_WRITE)

    set_renderer_timeout(120000)
