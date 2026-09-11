import logging
import os
import unittest

from slint_automation.log import LOGGER_NAME, configure_from_environment, logger


class LoggingChecks(unittest.TestCase):

    def test_logger_uses_namespaced_name(self) -> None:
        # act
        log = logger()
        # assert
        self.assertEqual(log.name, LOGGER_NAME)
        self.assertEqual(log.name, "slint_test")

    def test_configure_from_environment_enables_debug(self) -> None:
        # arrange
        os.environ["SLINT_TEST_DEBUG"] = "1"
        # arrange: capture the loggers for level restoration
        log = logging.getLogger(LOGGER_NAME)
        root = logging.getLogger()
        # arrange: restore the logger levels after the check
        self.addCleanup(log.setLevel, logging.NOTSET)
        self.addCleanup(root.setLevel, logging.WARNING)
        # act
        configure_from_environment()
        # assert
        self.assertEqual(log.level, logging.DEBUG)

    def test_configure_without_environment_keeps_default_level(self) -> None:
        # arrange
        os.environ.pop("SLINT_TEST_DEBUG", None)
        # arrange: capture the logger for level restoration
        log = logging.getLogger(LOGGER_NAME)
        # arrange: restore the logger level after the check
        self.addCleanup(log.setLevel, logging.NOTSET)
        # act
        configure_from_environment()
        # assert
        self.assertEqual(log.level, logging.NOTSET)
