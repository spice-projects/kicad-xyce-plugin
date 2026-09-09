# framework/slint_application.py
# SlintApplication manages the lifecycle of the Slint C++ application
# for testing purposes. It launches the process and provides a client
# for interacting with the UI.

import subprocess

from framework.slint_client import SlintClient


class SlintApplication:
    """Manages the lifecycle of the Slint application for testing."""

    def __init__(self, process, client):
        # m_process owns the OS process for the Slint application
        self._m_process = process
        # m_client provides UI interaction capabilities
        self._m_client = client

    def client(self):
        # Return the SlintClient instance for UI interaction
        return self._m_client

    def close(self):
        # Terminate the application process gracefully
        self._m_process.terminate()
        # Wait for the process to fully exit
        self._m_process.wait()

    def __enter__(self):
        # Support for 'with' statement context manager
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        # Ensure cleanup on context exit
        self.close()
        # Return False to propagate any exceptions
        return False


# ---------------------------------------------------------------------------

def launch(executable_path):
    # Launch the Slint application as a subprocess
    process = subprocess.Popen([executable_path])
    # Create the client wrapper
    client = SlintClient(process)
    # Return the application manager
    return SlintApplication(process, client)