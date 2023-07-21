"""
Process-related utility methods.
"""
import logging
import os
import subprocess


_LOGGER = logging.getLogger(__name__)


def execute_command(command, arguments, data=None, env=None, suppress_logging=False):
    """Execute a command, return result code, standard output, and standard error."""
    if env is None:
        env = {}
    assert command != ""
    _LOGGER.info(f"Executing '{command}' ")
    if not suppress_logging:
        _LOGGER.info(f" with {' '.join(arguments)}")

    environment = {**os.environ, **env}
    # pylint: disable=subprocess-run-check
    res = subprocess.run([command] + arguments, input=data, capture_output=True, env=environment)
    return [res.returncode, res.stdout, res.stderr]
