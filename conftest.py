import os
import sys
from unittest.mock import MagicMock

# mock wx to avoid errors in gh actions workflow
sys.modules["wx"] = MagicMock()

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "src"))

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
