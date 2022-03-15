"""Error related items"""


class ScriptException(RuntimeError):
    """Base Exception for this script"""


class OutOfIDsException(ScriptException):
    """Exception thrown when there are no more IDs that can be allocated"""

    def __init__(self, message):
        super().__init__(message.strip(), "Script has ran out of allocatable IDs")


class OutOfTAGsException(ScriptException):
    """Exception thrown when there are no more TAG IDs that can be allocated"""

    def __init__(self, message):
        super().__init__(message.strip(), "Script has ran out of allocatable TAG IDs")


class BlankTAGException(ScriptException):
    """Exception thrown when a TAG is blank"""

    def __init__(self, message):
        super().__init__(message.strip(), "TAG literal string cannot be blank")


class MalformedTAGDefinitionException(ScriptException):
    """Exception thrown when a TAG definition is malformed"""

    def __init__(self, message):
        super().__init__(message.strip(), "Implicit, single char or number definition of a LOG_TAG type")


class MalformedLogCallException(ScriptException):
    """Exception thrown when a Log call is malformed"""

    def __init__(self, message):
        super().__init__(message.strip(), "Implicit string or number inside a call to Log")


def error_to_string(error: Exception) -> Exception | str:
    """Split exception into string if it is a ScriptException, else, return the exception

    Args:
        error (Exception): Exception to interpret

    Returns:
        Exception | str: Result
    """
    if issubclass(type(error), ScriptException):
        return error.args[1] + "\n\t" + error.args[0]
    else:
        return error
