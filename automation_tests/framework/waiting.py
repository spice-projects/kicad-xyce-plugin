import time

from typing import Callable

from framework.errors import SlintAssertionError

DEFAULT_WAIT_TIMEOUT = 5.0
DEFAULT_POLL_INTERVAL = 0.1


def wait_for(condition: Callable[[], bool], *, timeout: float = DEFAULT_WAIT_TIMEOUT, poll_interval: float = DEFAULT_POLL_INTERVAL, timeout_message: str, observe: Callable[[], str] | None = None) -> None:
    # poll the condition until it holds or the timeout expires
    deadline = time.monotonic() + timeout
    while True:
        # evaluate the polled condition
        if condition():
            return
        # stop polling once the deadline expires
        if time.monotonic() >= deadline:
            break
        # sleep between polls
        time.sleep(poll_interval)
    # read the last observed state for the failure message
    state = _safe_observe(observe)
    # raise the final failure with the last observed state
    raise SlintAssertionError(f"{timeout_message}\nlast observed: {state}\ntimeout: {timeout} seconds")


def _safe_observe(observe: Callable[[], str] | None) -> str:
    # fall back to an unknown state when observation fails
    if observe is None:
        # report an unknown state without an observer
        return "unknown"
    try:
        # read the last observed state
        return observe()
    except Exception:
        # observation must never mask the timeout failure
        return "unknown"
