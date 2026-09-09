# tests/application_test.py
# Test cases for the SlintApplication testing framework
# Uses pytest fixtures to manage application lifecycle

import pytest

from framework import launch


@pytest.fixture
def app():
    # arrange: launch the application for testing
    with launch("./.build-debug/kicad-xyce-plugin") as app:
        yield app


def test_application_launch(app):
    # act & assert: verify the application launched successfully
    assert app is not None


def test_application_has_client(app):
    # act: get the client from the application
    client = app.client()
    # assert: verify the client is not None
    assert client is not None


def test_application_status(app):
    # act: check the application status
    client = app.client()
    status = client.get_status()
    # assert: verify the application is running
    assert status == "running"


def test_application_window(app):
    # act: get the application window
    client = app.client()
    window = client.get_window()
    # assert: verify window is retrieved (even if None in this stub)
    assert window is not None