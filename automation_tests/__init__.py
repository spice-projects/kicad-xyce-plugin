import sys

from pathlib import Path

# expose this package directory on the path so the framework and test modules
# import as top level packages under any unittest discovery root
sys.path.insert(0, str(Path(__file__).resolve().parent))
