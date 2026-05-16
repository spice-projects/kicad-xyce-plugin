import glob
import json
import os
import platform
import shutil
from dataclasses import dataclass
from pathlib import Path

from dataclasses_json import LetterCase, dataclass_json
from PySide6.QtCore import QSettings

_SETTINGS_ORGANIZATION = "KiCad"
_SETTINGS_APPLICATION = "XyceSimulatorPlugin"
_XYCE_EXECUTABLE_PATH_KEY = "xyceExecutablePath"


def discover_xyce_executable() -> str:
    # check PATH first (covers source builds, Spack, and custom installs)
    found = shutil.which("Xyce")
    # return immediately if the executable is on PATH
    if found:
        return found
    # determine the platform to select well-known install path patterns
    system = platform.system()
    # use Windows-specific install path pattern
    if system == "Windows":
        patterns = [r"C:\Program Files\XyceNF_*\bin\Xyce.exe"]
    else:
        # use the shared macOS and Linux binary installer directory layout
        patterns = ["/usr/local/XyceNF_*/bin/Xyce"]
    # collect all candidate paths matching the patterns
    candidates = []
    # extend candidates with matches for each pattern
    for pattern in patterns:
        # add glob matches to the candidate list
        candidates.extend(glob.glob(pattern))
    # prefer the highest version if any candidates were found
    if candidates:
        # sort candidates and return the last (highest version)
        return sorted(candidates)[-1]
    # no executable found, return empty string
    return ""


@dataclass_json(letter_case=LetterCase.CAMEL)
@dataclass(frozen=True)
class PluginConfig:

    xyce_executable_path: str

    @classmethod
    def load(cls, settings_path: str) -> "PluginConfig":
        # load configuration from the specified JSON file if it exists
        if os.path.isfile(settings_path):
            # open file for reading
            with open(settings_path, "r") as f:
                # parse JSON data
                data = json.load(f)
                # load configuration from the parsed data and return it
                return cls.from_dict(data)
        # create default instance
        return cls.default()

    @classmethod
    def default(cls) -> "PluginConfig":
        return cls(xyce_executable_path=discover_xyce_executable())

    def save(self) -> None:
        # open the same application settings scope used by load
        settings = QSettings(_SETTINGS_ORGANIZATION, _SETTINGS_APPLICATION)
        # persist the Xyce executable path value
        settings.setValue(_XYCE_EXECUTABLE_PATH_KEY, self.xyce_executable_path)

    def is_xyce_executable_valid(self) -> bool:
        # reject missing paths before filesystem checks
        if not self.xyce_executable_path:
            return False
        # create a path object for file existence checks
        executable_path = Path(self.xyce_executable_path)
        # reject paths that do not reference an existing file
        if not executable_path.is_file():
            return False
        # require executable permission to ensure the command can run
        return os.access(executable_path, os.X_OK)
