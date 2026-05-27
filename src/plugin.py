import logging
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional

from __version__ import __version__

logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s")

logger = logging.getLogger(__name__)

PLUGIN_ID = "com.github.spice-projects.kicad-xyce-plugin"

APP_AUTHOR = "Spice Projects"
APP_NAME = "com.github.spice-projects.kicad-xyce-plugin"

REQUIRED_PYTHON_VERSION = (3, 10)
WINDOWS_PYTHON_CANDIDATES = ("py",)
PYTHON_CANDIDATES = (
    "python",
    "python3",
    "python3.14",
    "python3.13",
    "python3.12",
    "python3.11",
    "python3.10",
)


def _get_user_data_dir() -> Path:
    # check if the current platform is windows
    if sys.platform == "win32":
        # use the appdata roaming directory on windows
        base_path = Path(os.environ.get("APPDATA", "~")).expanduser()
        # combine base path with author and application name
        path = base_path / APP_AUTHOR / APP_NAME
    # check if the current platform is macOS
    elif sys.platform == "darwin":
        # use the standard application support directory on macOS
        base_path = Path("~/Library/Application Support").expanduser()
        # combine base path with application name
        path = base_path / APP_NAME
    # handle linux and other unix-like platforms
    else:
        # use the xdg data home directory or default local share
        base_path = Path(os.environ.get("XDG_DATA_HOME", "~/.local/share")).expanduser()
        # combine base path with application name
        path = base_path / APP_NAME
    # return the resolved absolute path object
    return path.resolve()


# persistent application data directory calculated once at module load
APP_DIR = _get_user_data_dir()


def _show_error_dialog(message: str):
    # log error message to console/file for diagnostics
    logger.error("FATAL ERROR: %s", message)
    # define the title for the graphical error dialog
    title = "Xyce Simulation Plugin Error"
    # wrap system calls in a try block to handle environment differences
    try:
        # use applescript for a native alert on macOS
        if sys.platform == "darwin":
            # escape double quotes in the message for shell safety
            escaped_msg = message.replace('"', '\\"')
            # execute osascript to display a critical alert
            subprocess.run(["osascript", "-e", f'display alert "{title}" message "{escaped_msg}" as critical'], check=False)
        # use powershell for a native message box on windows
        elif sys.platform == "win32":
            # escape single quotes in the message for powershell safety
            escaped_msg = message.replace("'", "''")
            # build the powershell command to load winforms and show a message box
            ps_cmd = f"[Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms'); [Windows.Forms.MessageBox]::Show('{escaped_msg}', '{title}', [Windows.Forms.MessageBoxButtons]::OK, [Windows.Forms.MessageBoxIcon]::Error)"
            # execute powershell in non-interactive mode
            subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command", ps_cmd], check=False)
        # handle linux platforms using common desktop utilities
        else:
            # check if zenity utility is available in the path
            if shutil.which("zenity"):
                # show an error dialog using zenity
                subprocess.run(["zenity", "--error", f"--title={title}", f"--text={message}"], check=False)
            # fallback to notify-send if zenity is not present
            elif shutil.which("notify-send"):
                # show a critical desktop notification
                subprocess.run(["notify-send", "-u", "critical", title, message], check=False)
    except Exception:
        # log failure to display the graphical dialog
        logger.warning("Could not show system error dialog.", exc_info=True)


def _get_clean_env() -> dict[str, str]:
    # copy the current environment variables to a new dictionary
    env = os.environ.copy()
    # remove python related environment variables that may interfere with venv creation/usage
    env.pop("PYTHONPATH", None)
    # remove python home variable to prevent loading library from wrong location
    env.pop("PYTHONHOME", None)
    # return the filtered environment dictionary
    return env

ENV = _get_clean_env()

def _extract_python_version(python_path: Path) -> Optional[tuple[int, int]]:
    try:
        # build the command to query the candidate's Python version
        if python_path.name.lower() in ("py", "py.exe"):
            args = ["-3", "-c", "import sys; print(sys.version_info.major, sys.version_info.minor)"]
        else:
            args = ["-c", "import sys; print(sys.version_info.major, sys.version_info.minor)"]
        # execute command with a clean environment and capture output
        output = subprocess.check_output([str(python_path)] + args, env=ENV, text=True, stderr=subprocess.STDOUT, timeout=5)
        # parse the version numbers from the trimmed output
        parts = output.strip().split()
        # verify that output contains at least major and minor version parts
        if len(parts) < 2:
            # log failure to parse the output string
            logger.info("Failed to parse Python [%s] version from output: %s", python_path, output)
            # return none when version cannot be determined
            return None
        # return tuple of major and minor version numbers
        return (int(parts[0]), int(parts[1]))
    except (subprocess.CalledProcessError, FileNotFoundError, OSError, subprocess.TimeoutExpired, ValueError):
        # log execution or parsing error for the candidate executable
        logger.error("Error while analyzing Python executable: %s", python_path, exc_info=True)
        # return none on any failure
        return None


def _is_python_version_compatible(python_path: Path) -> tuple[bool, Optional[tuple[int, int]]]:
    # verify the candidate exists and can be executed
    if not python_path.exists() or not os.access(python_path, os.X_OK):
        # log that candidate was skipped
        logger.info("Python executable not found or not executable: %s", python_path)
        # return false with no version info
        return False, None
    # extract version using the helper function
    version = _extract_python_version(python_path)
    # check if version extraction succeeded
    if not version:
        # return false when version is unknown
        return False, None
    # extract version
    major = int(version[0])
    minor = int(version[1])
    # log information
    logger.info("Analyzing Python [%s], version: %d.%d", python_path, major, minor)
    # validate version is supported against the project requirement
    return (major, minor) >= REQUIRED_PYTHON_VERSION, (major, minor)


def _candidate_python_executables() -> list[Path]:
    # gather candidates from environment and runtime defaults
    candidates: list[Path] = []
    # check if a custom python path is defined in environment
    env_path = os.environ.get("PYTHON_PATH", "").strip()
    # add environment candidate if present
    if env_path:
        # append the custom path to the candidates list
        candidates.append(Path(env_path))
    # add the current python executable running this script
    if sys.executable:
        # append the current runtime path
        candidates.append(Path(sys.executable))
    # select candidate names based on the operating system
    if sys.platform.startswith("win"):
        # combine windows-specific and generic names
        candidate_names = WINDOWS_PYTHON_CANDIDATES + PYTHON_CANDIDATES
    else:
        # use generic python names on non-windows platforms
        candidate_names = PYTHON_CANDIDATES
    # loop through common executable names
    for name in candidate_names:
        # check if executable exists in system PATH
        resolved = shutil.which(name)
        # add resolved path to candidates list
        if resolved:
            # append path object for the found executable
            candidates.append(Path(resolved))
    # remove duplicate paths while preserving the original order
    unique_candidates: list[Path] = []
    # set to track seen absolute paths
    seen: set[str] = set()
    # iterate through all collected candidates
    for candidate in candidates:
        # resolve absolute path to handle symbolic links
        resolved = str(candidate.resolve())
        # skip if this path was already processed
        if resolved in seen:
            # continue to next candidate
            continue
        # log the candidate being considered
        logger.info("Considering Python executable at: %s", candidate)
        # mark path as seen in the tracking set
        seen.add(resolved)
        # add to the final unique list
        unique_candidates.append(candidate)
    # return the list of unique candidate paths
    return unique_candidates


def find_python_executable_path() -> tuple[Optional[Path], Optional[tuple[int, int]]]:
    # iterate through all available python candidates
    for candidate in _candidate_python_executables():
        # expand user paths like home directory tilde
        candidate_path = candidate.expanduser()
        # check if the candidate is compatible with requirements
        is_compatible, version = _is_python_version_compatible(candidate_path)
        # return the first match found
        if is_compatible:
            # return both path and version info
            return candidate_path, version
    # no compatible python was found across all candidates
    return None, None


def _ensure_python_venv(python_path: Path, version: tuple[int, int]):
    # check if the python virtual environment configuration already exists
    if (APP_DIR / "pyvenv.cfg").is_file():
        # determine the path to the python executable within the venv
        python_exe = APP_DIR / "Scripts" / "python.exe" if os.name == "nt" else APP_DIR / "bin" / "python"
        # verify that the existing venv version matches the requirement
        if _extract_python_version(python_exe) == version:
            # log that the existing venv is reused
            logger.info("Using existing Python virtual environment at: %s", APP_DIR)
            # return early as no creation is needed
            return
    # log that a new venv is being created
    logger.info("Creating Python virtual environment at: %s", APP_DIR)
    # create the virtual environment using the selected python interpreter
    subprocess.check_call([str(python_path), "-m", "venv", str(APP_DIR)], env=ENV)


def _ensure_application_installed() -> Optional[Path]:
    # wrap installation logic in a try block to handle failures
    try:
        # ensure the base directory exists
        APP_DIR.mkdir(parents=True, exist_ok=True)
        # log intent to check or install the application
        logger.info("Ensuring application with version [%s] is installed at: %s", __version__, APP_DIR)
        # check for the existence of the version marker file
        if (APP_DIR / "version.txt").is_file():
            # load file content to verify the installed version
            with (APP_DIR / "version.txt").open() as f:
                # read version string and strip whitespace
                installed_version = f.read().strip()
            # compare installed version with current project version
            if installed_version == __version__:
                # log that the application is already up to date
                logger.info("Application with version [%s] is already installed at: %s", __version__, APP_DIR)
                # return path to the python executable within the venv
                return APP_DIR / "Scripts" / "python.exe" if os.name == "nt" else APP_DIR / "bin" / "python"
        # find a compatible system python installation to use for the venv
        python_path, version = find_python_executable_path()
        # check if a suitable python was found
        if python_path is None:
            # log failure to locate python
            logger.error("Unable to locate a Python executable with version >= 3.10")
            # show error dialog to the user for immediate feedback
            _show_error_dialog("Unable to locate a compatible Python executable (version >= 3.10).\n\nPlease ensure Python is installed and available in your PATH.")
            # return none to indicate failure
            return None
        # log the path of the selected python interpreter
        logger.info("Found Python %d.%d executable at: %s", version[0], version[1], python_path)
        # create the virtual environment if it doesn't exist or is outdated
        _ensure_python_venv(python_path, version)
        # determine the path to pip within the virtual environment
        pip_exe = APP_DIR / "Scripts" / "pip.exe" if os.name == "nt" else APP_DIR / "bin" / "pip"
        # resolve the absolute path to the directory containing this script
        plugin_dir = Path(__file__).resolve().parent
        # search for the plugin wheel package in the plugin directory
        wheels = list(plugin_dir.glob("kicad_xyce_plugin-*-py3-none-any.whl"))
        # check if at least one wheel file was found
        if not wheels:
            # log that the required package is missing
            logger.error("Could not find plugin wheel file in %s", plugin_dir)
            # notify user of the missing package file
            _show_error_dialog(f"Failed to locate the plugin package (wheel file) in:\n{plugin_dir}")
            # return none to indicate failure
            return None
        # select the first wheel file found in the list
        wheel_path = wheels[0]
        # install the plugin wheel package into the virtual environment
        subprocess.check_call([str(pip_exe), "install", str(wheel_path)], env=ENV)
        # create the version marker file only after a successful installation
        (APP_DIR / "version.txt").write_text(__version__)
        # return the path to the python executable for launching the plugin
        return APP_DIR / "Scripts" / "python.exe" if os.name == "nt" else APP_DIR / "bin" / "python"
    except subprocess.CalledProcessError as e:
        # log detailed error information for the failed subprocess call
        logger.error("Failed to install application at: %s", APP_DIR, exc_info=True)
        # show a detailed error message to the user
        _show_error_dialog(f"Installation failed during setup of the virtual environment or package.\n\nError: {e}")
        # return none to indicate failure
        return None


def main():
    # wrap the main execution flow in a catch-all block
    try:
        # extract the KiCad API socket path from environment
        socket = os.environ.get("KICAD_API_SOCKET", "")
        # extract the KiCad API token from environment
        token = os.environ.get("KICAD_API_TOKEN", "")
        # check if the required KiCad environment variables are present
        if not socket or not token:
            # log that the plugin was launched outside of KiCad
            logger.error("Missing required environment variables: KICAD_API_SOCKET or KICAD_API_TOKEN")
            # return early
            return
        # ensure the plugin is installed and get the venv python path
        python_path = _ensure_application_installed()
        # check if installation and venv setup succeeded
        if python_path:
            # launch the plugin application module using the venv python
            subprocess.check_call([str(python_path), "-m", "kicad_xyce_plugin"], env=ENV)
    except Exception as e:
        # log any unexpected exception that occurred during startup
        logger.error("An unexpected error occurred in the plugin bootstrapper", exc_info=True)
        # show the unexpected error details to the user
        _show_error_dialog(f"An unexpected error occurred while starting the Xyce Simulation Plugin:\n\n{e}")


if __name__ == "__main__":
    # execute the main entry point
    main()
