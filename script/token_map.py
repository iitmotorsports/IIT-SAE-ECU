from tokenize import NUMBER, STRING, NAME, INDENT, ENDMARKER

_sel_end = {
    ')': {
        ';': ENDMARKER
    }
}

_sel_med = {
    ')': _sel_end[')'],
    ',': {
        NUMBER: _sel_end,
        NAME: _sel_end
    }
}

_sel_msg = {
    ',': {
        STRING: {
            ')': _sel_end[')'],
            ',': {
                NUMBER: _sel_med,
                NAME: _sel_med
            }
        }
    }
}

_sel_start = {
    '(': {
        STRING: _sel_msg,
        NAME: _sel_msg,
    }
}

sel_dict = {
    "Log": {
        '.': {
            'i': _sel_start,
            'e': _sel_start,
            'w': _sel_start,
            'd': _sel_start,
            'f': _sel_start,
            'p': _sel_start
        },
        '(': _sel_start['(']
    }
}


def get_type(tok: str) -> int | None:
    """Get if a token is a tokenize type

    Args:
        tok (str): The token

    Returns:
        int | None: int of the type
    """
    try:
        return tok if int(tok) in [NUMBER, STRING, NAME, INDENT] else None
    except ValueError:
        pass
