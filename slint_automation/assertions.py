from typing import Any, Callable

from .errors import LocatorError
from .locator import Locator
from .waiting import DEFAULT_POLL_INTERVAL, DEFAULT_WAIT_TIMEOUT, reports_test_frames, wait_for

DEFAULT_ASSERTION_TIMEOUT = DEFAULT_WAIT_TIMEOUT


def expect(locator: Locator) -> "Expect":
    # create an expectation builder for the locator
    return Expect(locator)


class Expect:

    def __init__(self, locator: Locator, negate: bool = False) -> None:
        # locator is the element under assertion
        self._locator = locator
        # negate inverts the assertion condition
        self._negate = negate

    @property
    def not_(self) -> "Expect":
        # return a negated expectation for the same locator
        return Expect(self._locator, not self._negate)

    @reports_test_frames
    def to_exist(self, *, timeout: float = DEFAULT_ASSERTION_TIMEOUT, poll_interval: float = DEFAULT_POLL_INTERVAL) -> None:
        # poll until the element existence matches the expectation
        wait_for(self._condition(lambda: self._locator.exists()), timeout=timeout, poll_interval=poll_interval, timeout_message=f"expected element {self._locator.describe()!r} {'not to exist' if self._negate else 'to exist'}", observe=lambda: "exists" if self._locator.exists() else "missing")

    @reports_test_frames
    def to_have_text(self, text: str, *, timeout: float = DEFAULT_ASSERTION_TIMEOUT, poll_interval: float = DEFAULT_POLL_INTERVAL) -> None:
        # poll until the element text matches the expectation
        wait_for(self._condition(lambda: self._locator.text() == text), timeout=timeout, poll_interval=poll_interval, timeout_message=f"expected element {self._locator.describe()!r} {'not to have text' if self._negate else 'to have text'} {text!r}", observe=lambda: f"text {self._locator.text()!r}")

    @reports_test_frames
    def to_have_property(self, name: str, expected: Any, *, timeout: float = DEFAULT_ASSERTION_TIMEOUT, poll_interval: float = DEFAULT_POLL_INTERVAL) -> None:
        # poll until the named property matches the expectation
        wait_for(self._condition(lambda: self._locator.property(name) == expected), timeout=timeout, poll_interval=poll_interval, timeout_message=f"expected element {self._locator.describe()!r} {'not to have property' if self._negate else 'to have property'} {name!r} == {expected!r}", observe=lambda: f"{name} {self._locator.property(name)!r}")

    @reports_test_frames
    def to_be_visible(self, *, timeout: float = DEFAULT_ASSERTION_TIMEOUT, poll_interval: float = DEFAULT_POLL_INTERVAL) -> None:
        # poll until the element opacity becomes positive
        wait_for(self._condition(lambda: self._opacity() > 0.0), timeout=timeout, poll_interval=poll_interval, timeout_message=f"expected element {self._locator.describe()!r} {'not to be visible' if self._negate else 'to be visible'}", observe=lambda: f"computed opacity {self._opacity()}")

    @reports_test_frames
    def to_be_enabled(self, *, timeout: float = DEFAULT_ASSERTION_TIMEOUT, poll_interval: float = DEFAULT_POLL_INTERVAL) -> None:
        # poll until the element reports the enabled flag
        wait_for(self._condition(self._enabled), timeout=timeout, poll_interval=poll_interval, timeout_message=f"expected element {self._locator.describe()!r} {'not to be enabled' if self._negate else 'to be enabled'}", observe=lambda: f"enabled {self._enabled()}")

    @reports_test_frames
    def to_have_opacity(self, expected: float, *, timeout: float = DEFAULT_ASSERTION_TIMEOUT, poll_interval: float = DEFAULT_POLL_INTERVAL) -> None:
        # poll until the element opacity matches the expectation
        wait_for(self._condition(lambda: abs(self._opacity() - expected) < 0.01), timeout=timeout, poll_interval=poll_interval, timeout_message=f"expected element {self._locator.describe()!r} {'not to have opacity' if self._negate else 'to have opacity'} {expected}", observe=lambda: f"opacity {self._opacity()}")

    def _condition(self, positive: Callable[[], bool]) -> Callable[[], bool]:
        # wrap the positive check with negation and absence handling
        def wrapped() -> bool:
            try:
                # evaluate the positive element state
                return positive() != self._negate
            except LocatorError:
                # an absent element satisfies negated expectations only
                return self._negate
        return wrapped

    def _opacity(self) -> float:
        # read the computed opacity where omitted zero values mean 0.0
        return float(self._locator.properties().get("computedOpacity", 0.0))

    def _enabled(self) -> bool:
        # read the enabled flag where omitted values mean false
        return bool(self._locator.properties().get("accessibleEnabled", False))
