import logging
import os
from pathlib import Path

logger = logging.getLogger(__name__)


def get_active_schematic_path() -> tuple[Path, float]:
    # 1. fetch the project directory set by KiCad
    prj_mod = os.environ.get("KIPRJMOD")
    if not prj_mod:
        raise RuntimeError("KIPRJMOD environment variable is not set. Is this script running inside KiCad?")
    # project path
    project_dir = Path(prj_mod)
    # 2. look for the .kicad_pro file to get the true project name
    project_files = list(project_dir.glob("*.kicad_pro"))
    if project_files:
        # schematic file with the same name as the project file is the root schematic
        root_schematic = project_dir / f"{project_files[0].stem}.kicad_sch"
        if root_schematic.exists():
            return root_schematic, root_schematic.stat().st_mtime
    # 3. fallback: If no .kicad_pro was found or the schematic has a unique name,
    schematic_files = list(project_dir.glob("*.kicad_sch"))
    if schematic_files:
        # In a standard project, the root sheet shares the project directory root level
        return schematic_files[0], schematic_files[0].stat().st_mtime
    # log information
    logger.info(f"No schematic file (.kicad_sch) found inside {project_dir}")
    # 4. if no schematic file is found, raise an error
    raise FileNotFoundError(f"No schematic file (.kicad_sch) found inside {project_dir}")
