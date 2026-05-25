import logging
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional

from platformdirs import user_data_dir

from __version__ import __version__

logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s")

logger = logging.getLogger(__name__)

PLUGIN_ID = "com.github.spice-projects.kicad-xyce-plugin"

APP_AUTHOR = "Spice Projects"
APP_NAME = "com.github.spice-projects.kicad-xyce-plugin"

APP_DIR = Path(user_data_dir(APP_NAME, APP_AUTHOR))

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


def _extract_python_version(python_path: Path) -> Optional[tuple[int, int]]:
    try:
        # build the command to query the candidate's Python version
        if python_path.name.lower() in ("py", "py.exe"):
            args = ["-3", "-c", "import sys; print(sys.version_info.major, sys.version_info.minor)"]
        else:
            args = ["-c", "import sys; print(sys.version_info.major, sys.version_info.minor)"]
        # execute command with a clean environment and capture output
        output = subprocess.check_output([str(python_path)] + args, env={"PATH": os.defpath}, text=True, stderr=subprocess.STDOUT, timeout=5)
        # parse the version numbers from output
        parts = output.strip().split()
        if len(parts) < 2:
            # log information
            logger.info("Failed to parse Python [%s] version from output: %s", python_path, output)
            # exit
            return None
        # return version
        return (int(parts[0]), int(parts[1]))
    except (subprocess.CalledProcessError, FileNotFoundError, OSError, subprocess.TimeoutExpired, ValueError):
        # log information
        logger.error("Error while analyzing Python executable: %s", python_path, exc_info=True)
        # exit
        return None


def _is_python_version_compatible(python_path: Path) -> tuple[bool, Optional[tuple[int, int]]]:
    # verify the candidate exists and can be executed
    if not python_path.exists() or not os.access(python_path, os.X_OK):
        # log information
        logger.info("Python executable not found or not executable: %s", python_path)
        # exit
        return False, None
    # extract version
    version = _extract_python_version(python_path)
    if not version:
        return False, None
    # extract version parts
    major = int(version[0])
    minor = int(version[1])
    # log information
    logger.info("Analyzing Python [%s], version: %d.%d", python_path, major, minor)
    # validate version is supported
    return (major, minor) >= REQUIRED_PYTHON_VERSION, (major, minor)


def _candidate_python_executables() -> list[Path]:
    # gather candidates from environment and runtime defaults
    candidates: list[Path] = []
    # check PYTHON_PATH is defined
    env_path = os.environ.get("PYTHON_PATH", "").strip()
    if env_path:
        candidates.append(Path(env_path))
    # current python, the one running this script
    if sys.executable:
        candidates.append(Path(sys.executable))
    # candidates by platform
    if sys.platform.startswith("win"):
        candidate_names = WINDOWS_PYTHON_CANDIDATES + PYTHON_CANDIDATES
    else:
        candidate_names = PYTHON_CANDIDATES
    # loop candidates
    for name in candidate_names:
        # check exists in PATH
        resolved = shutil.which(name)
        if resolved:
            candidates.append(Path(resolved))
    # remove duplicate paths while preserving the original order
    unique_candidates: list[Path] = []
    seen: set[str] = set()
    for candidate in candidates:
        # resolve absolute path
        resolved = str(candidate.resolve())
        if resolved in seen:
            continue
        # log information
        logger.info("Considering Python executable at: %s", candidate)
        # append it as seen
        seen.add(resolved)
        unique_candidates.append(candidate)
    # return list of candidates
    return unique_candidates


def find_python_executable_path() -> tuple[Optional[Path], Optional[tuple[int, int]]]:
    # return the first compatible local Python executable
    for candidate in _candidate_python_executables():
        # expand user paths
        candidate_path = candidate.expanduser()
        # check it is compatible with this plugin code
        is_compatible, version = _is_python_version_compatible(candidate_path)
        if is_compatible:
            return candidate_path, version
    # no python was found
    return None, None


def _ensure_python_venv(python_path: Path, version: tuple[int, int]):
    # check python venv exists
    if (APP_DIR / "pyvenv.cfg").is_file():
        # python exe within venv
        python_exe = APP_DIR / "Scripts" / "python.exe" if os.name == "nt" else APP_DIR / "bin" / "python"
        # verify version
        if _extract_python_version(python_exe) == version:
            # log information
            logger.info("Using existing Python virtual environment at: %s", APP_DIR)
            # exit
            return
    # log information
    logger.info("Creating Python virtual environment at: %s", APP_DIR)
    # create the virtual environment
    subprocess.check_call([str(python_path), "-m", "venv", str(APP_DIR)], env={"PATH": os.defpath})


def _ensure_application_installed() -> Optional[Path]:
    try:
        # ensure the base directory exists
        APP_DIR.mkdir(parents=True, exist_ok=True)
        # log information
        logger.info("Ensuring application with version [%s] is installed at: %s", __version__, APP_DIR)
        # load version.txt from application install dir
        if (APP_DIR / "version.txt").is_file():
            # load file content
            with (APP_DIR / "version.txt").open() as f:
                installed_version = f.read().strip()
            # compare version
            if installed_version == __version__:
                #  log information
                logger.info("Application with version [%s] is already installed at: %s", __version__, APP_DIR)
                # exit, nothing to do
                return APP_DIR / "Scripts" / "python.exe" if os.name == "nt" else APP_DIR / "bin" / "python"
        # find python installation to use
        python_path, version = find_python_executable_path()
        if python_path is None:
            # log information
            logger.error("Unable to locate a Python executable with version >= 3.10")
            # exit
            return None
        # log information
        logger.info("Found Python %d.%d executable at: %s", version[0], version[1], python_path)
        # create the virtual environment if it doesn't exist
        _ensure_python_venv(python_path, version)
        # python exe within venv
        pip_exe = APP_DIR / "Scripts" / "pip.exe" if os.name == "nt" else APP_DIR / "bin" / "pip"
        # install wheel package
        subprocess.check_call([str(pip_exe), "install", f"kicad_xyce_plugin-{__version__}-py3-none-any.whl"], env={"PATH": os.defpath})
        # create version file in install dir
        (APP_DIR / "version.txt").write_text(__version__)
        # exit
        return APP_DIR / "Scripts" / "python.exe" if os.name == "nt" else APP_DIR / "bin" / "python"
    except subprocess.CalledProcessError:
        # log information
        logger.error("Failed to install application at: %s", APP_DIR)
        # exit
        return None


def main():
    # extract required environment variables
    socket = os.environ.get("KICAD_API_SOCKET", "")
    token = os.environ.get("KICAD_API_TOKEN", "")
    if not socket or not token:
        # log information
        logger.error("Missing required environment variables: KICAD_API_SOCKET or KICAD_API_TOKEN")
        # exit
        return
    # ensure application is installed
    python_path = _ensure_application_installed()
    if python_path:
        # launch application
        subprocess.check_call([str(python_path), "-m", "kicad_xyce_plugin"], env={"PATH": os.defpath, "KICAD_API_SOCKET": socket, "KICAD_API_TOKEN": token})


if __name__ == "__main__":
    main()
