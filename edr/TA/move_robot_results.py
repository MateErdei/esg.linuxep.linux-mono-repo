import os
import shutil

if __name__ == '__main__':
    robot_logs = 'log.html'
    robot_output = 'output.xml'
    robot_report = 'report.html'

    robot_results_folder = '/'
    tap_logs_dir = '/opt/test/logs'

    robot_results = [robot_logs, robot_output, robot_report]

    for robot_result_file in robot_results:
        dest_file_path = os.path.join(tap_logs_dir, robot_result_file)
        if os.path.exists(dest_file_path):
            os.remove(dest_file_path)
        shutil.move(os.path.join(robot_results_folder,robot_result_file), tap_logs_dir)

