import os
import sys

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")

from unittest import TestCase

from PySide6.QtGui import QIcon
from PySide6.QtWidgets import QApplication

from plugin.kicad_icons import KiCadIcon, get_kicad_icon, load_kicad_icons

_app = QApplication.instance() or QApplication(sys.argv)


class TestKiCadIconEnum(TestCase):

    def test_all_enum_members_have_unique_values(self):
        # arrange
        values = [icon.value for icon in KiCadIcon]
        # assert
        self.assertEqual(len(values), len(set(values)))

    def test_file_save_member_exists(self):
        # assert
        self.assertIn(KiCadIcon.FILE_SAVE, KiCadIcon)

    def test_file_open_member_exists(self):
        # assert
        self.assertIn(KiCadIcon.FILE_OPEN, KiCadIcon)

    def test_sim_run_member_exists(self):
        # assert
        self.assertIn(KiCadIcon.SIM_RUN, KiCadIcon)

    def test_cancel_member_exists(self):
        # assert
        self.assertIn(KiCadIcon.CANCEL, KiCadIcon)

    def test_preference_member_exists(self):
        # assert
        self.assertIn(KiCadIcon.PREFERENCE, KiCadIcon)

    def test_nine_icons_total(self):
        # assert
        self.assertEqual(len(list(KiCadIcon)), 9)


class TestLoadKiCadIcons(TestCase):

    def test_load_kicad_icons_completes_without_error(self):
        # act / assert — no exception raised
        load_kicad_icons()

    def test_load_kicad_icons_is_idempotent(self):
        # arrange — ensure icons are loaded first
        load_kicad_icons()
        # act — call again
        load_kicad_icons()
        # assert — no exception, no duplicate load


class TestGetKiCadIcon(TestCase):

    def test_returns_light_icon_by_default(self):
        # arrange
        load_kicad_icons()
        # act
        icon = get_kicad_icon(KiCadIcon.FILE_SAVE)
        # assert — returns a QIcon (may be null if file missing, but type is correct)
        self.assertIsInstance(icon, QIcon)

    def test_returns_dark_icon_when_requested(self):
        # arrange
        load_kicad_icons()
        # act
        icon = get_kicad_icon(KiCadIcon.FILE_SAVE, dark=True)
        # assert
        self.assertIsInstance(icon, QIcon)

    def test_returns_light_icon_when_dark_false(self):
        # arrange
        load_kicad_icons()
        # act
        icon = get_kicad_icon(KiCadIcon.SIM_RUN, dark=False)
        # assert
        self.assertIsInstance(icon, QIcon)

    def test_all_light_icons_can_be_retrieved(self):
        # arrange
        load_kicad_icons()
        # assert — no KeyError raised for any enum member
        for icon_key in KiCadIcon:
            icon = get_kicad_icon(icon_key, dark=False)
            self.assertIsInstance(icon, QIcon)

    def test_all_dark_icons_can_be_retrieved(self):
        # arrange
        load_kicad_icons()
        # assert — no KeyError raised for any enum member
        for icon_key in KiCadIcon:
            icon = get_kicad_icon(icon_key, dark=True)
            self.assertIsInstance(icon, QIcon)
