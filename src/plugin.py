import logging
import os
import shutil
import subprocess
import sys
import threading
from pathlib import Path
from typing import Optional

import wx

from __version__ import __version__


# configure global logging
logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(name)s - %(levelname)s - %(message)s")
# create logger for this module
logger = logging.getLogger(__name__)
# application author for data directories
APP_AUTHOR = "Spice Projects"
# unique plugin identifier
PLUGIN_ID = "com_github_spice-projects_kicad-xyce-plugin"
# application name for data directories
APP_NAME = "com.github.spice-projects.kicad-xyce-plugin"
# minimum required python version
REQUIRED_PYTHON_VERSION = (3, 10)
# candidate executables for windows launcher
WINDOWS_PYTHON_CANDIDATES = ("py",)
# common python executable names to search
PYTHON_CANDIDATES = ("python", "python3", "python3.14", "python3.13", "python3.12", "python3.11", "python3.10")
# common python install directories on macos not always present in kicad's restricted path
# (kicad launched via finder/dock does not inherit the user's shell PATH)
MACOS_PYTHON_SEARCH_DIRS = (
    "/opt/homebrew/bin",   # apple silicon homebrew
    "/usr/local/bin",      # intel homebrew and standard installs
    "/usr/bin",            # system python
)


def _get_user_data_dir() -> Path:
    # check if platform is windows
    if sys.platform == "win32":
        # resolve windows appdata directory
        path = Path(os.environ.get("APPDATA", "~")).expanduser() / APP_AUTHOR / APP_NAME
    # check if platform is macos
    elif sys.platform == "darwin":
        # resolve macos application support directory
        path = Path("~/Library/Application Support").expanduser() / APP_NAME
    # handle other platforms
    else:
        # resolve xdg data home or local share directory
        path = Path(os.environ.get("XDG_DATA_HOME", "~/.local/share")).expanduser() / APP_NAME
    # return absolute resolved path
    return path.resolve()


# persistent application directory path
APP_DIR = _get_user_data_dir()


def _show_error_dialog(message: str):
    # log error message for diagnostics
    logger.error("FATAL ERROR: %s", message)
    # dialog title string
    title = "Xyce Simulation Plugin Error"
    # attempt to use wxpython for native error dialog
    try:
        # get active app or create a temporary one
        _ = wx.GetApp() or wx.App(False)
        # show modal message box
        wx.MessageBox(message, title, wx.OK | wx.ICON_ERROR)
        # return after showing dialog
        return
    # catch exceptions during wx usage
    except Exception:
        # log failure to use wx
        logger.warning("Could not show wx error dialog, falling back to system tools.", exc_info=True)
    # fall back to system-specific tools
    try:
        # handle macos platform
        if sys.platform == "darwin":
            # escape double quotes for applescript
            msg_esc = message.replace('"', '\\"')
            # run osascript alert
            subprocess.run(["osascript", "-e", f'display alert "{title}" message "{msg_esc}" as critical'], check=False)
        # handle windows platform
        elif sys.platform == "win32":
            # escape single quotes for powershell
            msg_esc = message.replace("'", "''")
            # build powershell messagebox command
            ps_cmd = f"[Reflection.Assembly]::LoadWithPartialName('System.Windows.Forms'); [Windows.Forms.MessageBox]::Show('{msg_esc}', '{title}', [Windows.Forms.MessageBoxButtons]::OK, [Windows.Forms.MessageBoxIcon]::Error)"
            # run powershell command
            subprocess.run(["powershell", "-NoProfile", "-NonInteractive", "-Command", ps_cmd], check=False)
        # handle linux and others
        else:
            # check for zenity utility
            if shutil.which("zenity"):
                # show zenity error dialog
                subprocess.run(["zenity", "--error", f"--title={title}", f"--text={message}"], check=False)
            # check for notify-send utility
            elif shutil.which("notify-send"):
                # show critical notification
                subprocess.run(["notify-send", "-u", "critical", title, message], check=False)
    # catch failures in system tool execution
    except Exception:
        # log failure to display system dialog
        logger.warning("Could not show system error dialog.", exc_info=True)


def _get_clean_env() -> dict[str, str]:
    # copy existing environment
    env = os.environ.copy()
    # remove pythonpath to prevent interference
    env.pop("PYTHONPATH", None)
    # remove pythonhome to prevent interference
    env.pop("PYTHONHOME", None)
    # return filtered environment
    return env


# base environment for subprocess calls
ENV = _get_clean_env()


def _extract_python_version(python_path: Path) -> Optional[tuple[int, int]]:
    # try to query version from executable
    try:
        # check if using windows launcher
        if python_path.name.lower() in ("py", "py.exe"):
            # build command with windows launcher flags
            args = ["-3", "-c", "import sys; print(sys.version_info.major, sys.version_info.minor)"]
        else:
            # build version query command
            args = ["-c", "import sys; print(sys.version_info.major, sys.version_info.minor)"]
        # execute command and capture output
        output = subprocess.check_output([str(python_path)] + args, env=ENV, text=True, stderr=subprocess.STDOUT, timeout=5)
        # split output into parts
        parts = output.strip().split()
        # verify at least two parts found
        if len(parts) < 2:
            # log failure to parse version
            logger.info("Failed to parse Python [%s] version from output: %s", python_path, output)
            # return none on parse failure
            return None
        # return major and minor version tuple
        return (int(parts[0]), int(parts[1]))
    # catch execution and timeout errors
    except (subprocess.CalledProcessError, FileNotFoundError, OSError, subprocess.TimeoutExpired, ValueError):
        # log error during analysis
        logger.error("Error while analyzing Python executable: %s", python_path, exc_info=True)
        # return none on failure
        return None


def _is_python_version_compatible(python_path: Path) -> tuple[bool, Optional[tuple[int, int]]]:
    # check if path exists and is executable
    if not python_path.exists() or not os.access(python_path, os.X_OK):
        # log missing or inaccessible file
        logger.info("Python executable not found or not executable: %s", python_path)
        # return failure with no version
        return False, None
    # extract version from path
    version = _extract_python_version(python_path)
    if not version:
        # return false with no version
        return False, None
    # assign major and minor versions
    major, minor = int(version[0]), int(version[1])
    # log candidate analysis results
    logger.info("Analyzing Python [%s], version: %d.%d", python_path, major, minor)
    # return compatibility check and version tuple
    return (major, minor) >= REQUIRED_PYTHON_VERSION, (major, minor)


def _candidate_python_executables() -> list[Path]:
    # initialize candidates list
    candidates = []
    # get python path from environment
    env_path = os.environ.get("PYTHON_PATH", "").strip()
    # add env path if present
    if env_path:
        # append environment candidate
        candidates.append(Path(env_path))
    # add current runtime executable
    if sys.executable:
        # append runtime candidate
        candidates.append(Path(sys.executable))
    # determine names to search based on platform
    candidate_names = WINDOWS_PYTHON_CANDIDATES + PYTHON_CANDIDATES if sys.platform.startswith("win") else PYTHON_CANDIDATES
    # iterate through candidate names
    for name in candidate_names:
        # search for executable in path
        resolved = shutil.which(name)
        # add if found
        if resolved:
            # append resolved path
            candidates.append(Path(resolved))
    # on macos, kicad may be launched with a restricted PATH (via finder/dock) that
    # excludes homebrew and other common install locations; probe them directly
    if sys.platform == "darwin":
        # iterate through known python install directories
        for search_dir in MACOS_PYTHON_SEARCH_DIRS:
            # iterate through candidate names
            for name in PYTHON_CANDIDATES:
                # build full path to candidate
                path = Path(search_dir) / name
                # add if the file exists
                if path.is_file():
                    # append discovered path
                    candidates.append(path)
    # initialize unique list and tracking set
    unique_candidates, seen = [], set()
    # iterate through all candidates
    for candidate in candidates:
        # resolve absolute path for uniqueness check
        resolved = str(candidate.resolve())
        # check if already seen
        if resolved not in seen:
            # log candidate being considered
            logger.info("Considering Python executable at: %s", candidate)
            # mark as seen
            seen.add(resolved)
            # add to unique list
            unique_candidates.append(candidate)
    # return list of unique paths
    return unique_candidates


def find_python_executable_path() -> tuple[Optional[Path], Optional[tuple[int, int]]]:
    # iterate through unique candidates
    for candidate in _candidate_python_executables():
        # check compatibility of expanded path
        is_compatible, version = _is_python_version_compatible(candidate.expanduser())
        # return first compatible match
        if is_compatible:
            # return path and version
            return candidate.expanduser(), version
    # return none if no compatible match found
    return None, None


def _ensure_python_venv(python_path: Path, version: tuple[int, int]):
    # check if venv config exists
    if (APP_DIR / "pyvenv.cfg").is_file():
        # resolve venv python executable path
        python_exe = APP_DIR / "Scripts" / "python.exe" if os.name == "nt" else APP_DIR / "bin" / "python"
        # verify venv version matches requirement
        if _extract_python_version(python_exe) == version:
            # log reuse of existing venv
            logger.info("Using existing Python virtual environment at: %s", APP_DIR)
            # exit without creating new venv
            return
    # log venv creation attempt
    logger.info("Creating Python virtual environment at: %s", APP_DIR)
    # create new virtual environment
    subprocess.check_call([str(python_path), "-m", "venv", str(APP_DIR)], env=ENV)


def _ensure_application_installed() -> Optional[Path]:
    try:
        # create application directory
        APP_DIR.mkdir(parents=True, exist_ok=True)
        # log installation intent
        logger.info("Ensuring application with version [%s] is installed at: %s", __version__, APP_DIR)
        # resolve internal python executable path
        python_exe = APP_DIR / "Scripts" / "python.exe" if os.name == "nt" else APP_DIR / "bin" / "python"
        # check for existing version marker
        if (APP_DIR / "version.txt").is_file():
            # check if installed version matches current version
            if (APP_DIR / "version.txt").read_text().strip() == __version__:
                # log that setup is already complete
                logger.info("Application with version [%s] is already installed at: %s", __version__, APP_DIR)
                # return path to venv python
                return python_exe
        # get the existing wx application instance from kicad or create a temporary one
        _ = wx.GetApp() or wx.App(False)
        # initialize progress dialog; PD_AUTO_HIDE dismisses the dialog automatically
        # when Update(maximum) is called, avoiding the "waiting for dismiss" state
        # that occurs without this flag and prevents the dialog from closing
        progress = wx.ProgressDialog("Xyce Simulation Plugin Setup", "Initializing setup...", maximum=100, style=wx.PD_APP_MODAL | wx.PD_SMOOTH | wx.PD_AUTO_HIDE)
        # signals worker completion to the main thread
        done_event = threading.Event()
        # shared state between main thread and worker
        state = {"val": 0, "msg": "Initializing setup...", "error": None, "result": None}

        # background setup worker function
        def worker():
            # wrap worker logic in error handler
            try:
                # update state for python search
                state["val"], state["msg"] = 10, "Searching for a compatible Python 3.10+ interpreter..."
                # perform system-wide python search
                python_path, version = find_python_executable_path()
                # handle search failure
                if python_path is None:
                    # raise error when no compatible python found
                    raise RuntimeError("Unable to locate a compatible Python executable (version >= 3.10).\n\nPlease ensure Python is installed and available in your PATH.")
                # update state for venv creation
                state["val"], state["msg"] = 30, f"Preparing dedicated virtual environment with Python {version[0]}.{version[1]}..."
                # initialize virtual environment
                _ensure_python_venv(python_path, version)
                # resolve pip module path
                pip_exe = APP_DIR / "Scripts" / "pip.exe" if os.name == "nt" else APP_DIR / "bin" / "python"
                # find plugin wheel files
                wheels = list(Path(__file__).resolve().parent.glob("kicad_xyce_plugin-*-py3-none-any.whl"))
                # handle missing package files
                if not wheels:
                    # raise error when package not found
                    raise RuntimeError(f"Failed to locate the plugin package (wheel file) in:\n{Path(__file__).resolve().parent}")
                # update state for package installation
                state["val"], state["msg"] = 60, "Installing Xyce Simulation Plugin and dependencies..."
                # install plugin package via pip
                subprocess.check_call([str(pip_exe), "-m", "pip", "install", str(wheels[0])], env=ENV)
                # update state for finalization
                state["val"], state["msg"] = 90, "Finalizing installation..."
                # record installed version marker
                (APP_DIR / "version.txt").write_text(__version__)
                # store successful result
                state["result"] = python_exe
            except Exception as e:
                # log information
                logger.error("Installation worker failed", exc_info=True)
                # record error in shared state
                state["error"] = e
            finally:
                # signal main thread that worker is done
                done_event.set()

        # create and start setup thread, daemonized to exit with main thread
        thread = threading.Thread(target=worker, daemon=True)
        thread.start()
        # poll worker state while keeping the wx event loop alive.
        # done_event.wait releases the GIL so the worker runs freely during each interval.
        # wx.YieldIfNeeded has a re-entrancy guard and does not re-enable disabled windows,
        # making it safe to call inside a wx.PD_APP_MODAL dialog without disturbing the
        # modal loop or triggering spurious completion events (unlike wx.SafeYield).
        while not done_event.wait(timeout=0.05):
            # update graphical progress dialog
            progress.Update(state["val"], state["msg"])
            # process pending wx events so the dialog can repaint
            wx.YieldIfNeeded()
        # handle worker error: close dialog first, then show error so there is no
        if state["error"]:
            # hide dialog window
            progress.Hide()
            # destroy dialog resources
            progress.Destroy()
            # flush pending wx events so the dialog is actually removed from screen
            wx.Yield()
            # report captured error
            _show_error_dialog(str(state["error"]))
            # return failure
            return None
        # update to maximum triggers PD_AUTO_HIDE which dismisses the dialog
        progress.Update(100, "Setup complete!")
        # destroy dialog resources
        progress.Destroy()
        # flush pending wx events to finalize destruction
        wx.Yield()
        # return path to venv python
        return state["result"]
    # catch top-level setup errors
    except Exception as e:
        # log failure details
        logger.error("Failed to install application at: %s", APP_DIR, exc_info=True)
        # report unexpected error
        _show_error_dialog(f"An unexpected error occurred during setup.\n\nError: {e}")
        # return failure
        return None


def main():
    # execute main logic in error handler
    try:
        # get kicad api env vaariables
        socket, token, project_path = os.environ.get("KICAD_API_SOCKET", ""), os.environ.get("KICAD_API_TOKEN", ""), os.environ.get("KIPRJMOD", "")
        # verify api environment
        if not socket or not token or not project_path:
            # log launch outside of kicad
            logger.error("Missing required environment variables: KICAD_API_SOCKET or KICAD_API_TOKEN or KIPRJMOD")
            # exit without error dialog
            return
        # log connection details
        logger.info("KICAD_API_SOCKET: %s", socket)
        logger.info("KICAD_API_TOKEN: %s", token)
        logger.info("KIPRJMOD: %s", project_path)
        # ensure environment is ready
        python_path = _ensure_application_installed()
        # launch plugin if setup succeeded
        if python_path:
            # start plugin module asynchronously and detach IO from this process
            subprocess.Popen([str(python_path), "-m", "kicad_xyce_plugin"], env=ENV, stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, close_fds=True, start_new_session=True)
    except Exception as e:
        # log fatal bootstrapper error
        logger.error("An unexpected error occurred in the plugin bootstrapper", exc_info=True)
        # report fatal error
        _show_error_dialog(f"An unexpected error occurred while starting the Xyce Simulation Plugin:\n\n{e}")


if __name__ == "__main__":
    # run main entry point
    main()
