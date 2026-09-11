import logging
import os

LOGGER_NAME = "slint_test"
DEBUG_ENVIRONMENT_VARIABLE = "SLINT_TEST_DEBUG"


def logger() -> logging.Logger:
    # return the framework namespaced logger
    return logging.getLogger(LOGGER_NAME)


def configure_from_environment() -> None:
    # enable debug protocol logging when the environment requests it
    if os.environ.get(DEBUG_ENVIRONMENT_VARIABLE):
        # attach a root handler at debug level when none is configured
        logging.basicConfig(level=logging.DEBUG)
        # force the framework logger down to debug level
        logging.getLogger(LOGGER_NAME).setLevel(logging.DEBUG)
