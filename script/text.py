def error(text):
    return f"\033[91m\033[1m\033[4m{text}\033[0m"


def underline(text):
    return f"\033[4m{text}\033[0m"


def header(text):
    return f"\033[1m\033[4m{text}\033[0m"


def warning(text):
    return f"\033[93m\033[1m{text}\033[0m"


def yellow(text):
    return f"\033[93m{text}\033[0m"


def important(text):
    return f"\033[94m\033[1m{text}\033[0m"


def reallyImportant(text):
    return f"\033[94m\033[1m\033[4m{text}\033[0m"


def green(text):
    return f"\033[92m{text}\033[0m"


def red(text):
    return f"\033[91m{text}\033[0m"
