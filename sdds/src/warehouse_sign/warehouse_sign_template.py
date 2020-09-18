import datetime
import os
import subprocess
import sys

# NOTE: These variables are replaced with BASE_PATH={git repo folder}, ENVIRONMENT={build os.environ}
ROOT_PATH = r'<BASE_PATH>'  # Repo path (where the warehouse_sign script is)
ENVIRONMENT = {'<TEMPLATE': 'ENV>'}  # Environment necessary for the warehouse_sign tools to work

if __name__ == '__main__':
    with open(ROOT_PATH + r'\logs\signlog.txt', 'a') as f:
        f.write(str(datetime.datetime.now()) + ' ' + str(sys.argv) + '\n')

        # The renderer is 32 bit, "py -3" is 64 bit, so fix any relative paths
        path_prefix = '' if os.path.isabs(sys.argv[1]) else 'C:\\Windows\\SysWOW64\\'

        args = [
            'py', '-3', ROOT_PATH + r'\src\warehouse_sign\sign.py',
            path_prefix + sys.argv[1],
            path_prefix + sys.argv[2]
        ]

        f.write(' '.join(args) + '\n')

        env = ENVIRONMENT.copy()
        env['PYTHONPATH'] = os.path.join(ROOT_PATH, 'src')
        subprocess.check_call(args, stdout=f, stderr=f, env=env)
        f.write('\n')
