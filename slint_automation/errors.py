class SlintTestError(Exception):
    pass


class ApplicationStartupError(SlintTestError):
    pass


class McpError(SlintTestError):
    pass


class LocatorError(SlintTestError):
    pass


class SlintAssertionError(AssertionError):
    pass
