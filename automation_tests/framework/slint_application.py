# framework/slint_application.py
# SlintApplication manages the lifecycle of the Slint C++ application
# for testing purposes. It launches the process and provides a client
# for interacting with the UI.

import subprocess

from framework.slint_client import SlintClient


class SlintApplication:
    def __init__(self, process, client):
        # process owns the OS process for the Slint application
        self._process = process
        # client provides UI interaction capabilities
        self._client = client

    def client(self):
        # return the SlintClient instance for UI interaction
        return self._client

    def close(self):
        # terminate the application process gracefully
        self._process.terminate()
        # wait for the process to fully exit
        self._process.wait()

    def __enter__(self):
        # support for 'with' statement context manager
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        # ensure cleanup on context exit
        self.close()
        # return False to propagate any exceptions
        return False


def launch(executable_path):
    # launch the Slint application as a subprocess
    process = subprocess.Popen([executable_path])
    # create the client wrapper
    client = SlintClient(process)
    # return the application manager
    return SlintApplication(process, client)
