"""Text styling functions"""


def error(text: str) -> str:
    """Formats text as an error

    Args:
        text (str): The text to format

    Returns:
        str: Formatted Text
    """
    return f"\033[91m\033[1m\033[4m{text}\033[0m"


def underline(text: str) -> str:
    """Formats text as underlined

    Args:
        text (str): The text to format

    Returns:
        str: Formatted Text
    """
    return f"\033[4m{text}\033[0m"


def header(text: str) -> str:
    """Formats text as a header

    Args:
        text (str): The text to format

    Returns:
        str: Formatted Text
    """
    return f"\033[1m\033[4m{text}\033[0m"


def warning(text: str) -> str:
    """Formats text as a warning

    Args:
        text (str): The text to format

    Returns:
        str: Formatted Text
    """
    return f"\033[93m\033[1m{text}\033[0m"


def yellow(text: str) -> str:
    """Formats text to be yellow

    Args:
        text (str): The text to format

    Returns:
        str: Formatted Text
    """
    return f"\033[93m{text}\033[0m"


def important(text: str) -> str:
    """Formats text as important

    Args:
        text (str): The text to format

    Returns:
        str: Formatted Text
    """
    return f"\033[94m\033[1m{text}\033[0m"


def really_important(text: str) -> str:
    """Formats text as really important

    Args:
        text (str): The text to format

    Returns:
        str: Formatted Text
    """
    return f"\033[94m\033[1m\033[4m{text}\033[0m"


def green(text: str) -> str:
    """Formats text to be green

    Args:
        text (str): The text to format

    Returns:
        str: Formatted Text
    """
    return f"\033[92m{text}\033[0m"


def red(text: str) -> str:
    """Formats text to be red

    Args:
        text (str): The text to format

    Returns:
        str: Formatted Text
    """
    return f"\033[91m{text}\033[0m"
