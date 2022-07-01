"""Script for parsing an SDBC file"""

from io import BytesIO
from tokenize import TokenInfo, tokenize
import tokenize as tkn
from typing import Dict, Generator, List, Set, TextIO, Tuple


def listify(lst: List, joiner=", ") -> str:
    """Formats a list for printing

    Args:
        lst (List): The list to format
        joiner (str, optional): List item separator. Defaults to ", ".

    Returns:
        str: Formatted list
    """
    return joiner.join([str(x) for x in lst])


def norm(tk: TokenInfo) -> str:
    """Normalizes a token

    Args:
        tk (TokenInfo): The token to normalize

    Returns:
        str: Normalized token
    """
    return tk.string.strip('"').upper()


def next_num(tks: Generator[TokenInfo, None, None]) -> float:
    """Gets the next number, uses up to two tokens

    Args:
        tks (Generator[TokenInfo, None, None]): The token generator

    Returns:
        float: number as a float, error if not found
    """
    frst = next(tks).string

    if frst == "-" or frst == "+":
        frst = frst + next(tks).string

    return float(frst)


class Type:
    """Type class for defining and verifying types"""

    name: str = None
    bits: int = None
    line: int = None

    def __init__(self, line: int, name: str, bits: int) -> None:
        self.line = line
        self.name = str(name)
        self.bits = int(bits)

    def __str__(self) -> str:
        return f"{self.name}@{self.bits}"

    def verify(self) -> None:
        """Verifies this type"""
        assert self.bits <= 64, f"Type exceeds 64 bits : {self.line}| {self.name}:{self.bits}"
        assert self.bits > 0, f"Type has invalid number of bits : {self.line}| {self.name}:{self.bits}"


class Format:
    """Format class for defining and verifying formats"""

    name: str = None
    signed: bool = False
    type: Type = None
    comment: str = None
    line: int = None

    def __init__(self, line: int, name: str, signed: bool, type: Type, comment: str) -> None:
        self.line = line
        self.name = str(name)
        self.signed = bool(signed)
        self.type = type
        self.comment = str(comment)

    def __str__(self) -> str:
        return f"{self.name} {'signed' if self.signed else 'unsigned'} {str(self.type)}"

    def verify(self, types: Dict[str, Type]) -> None:
        """Verifies this format, matching it's type, should only be called once

        Args:
            types (Dict[str, Type]): Defined Types
        """
        assert self.type in types, f"Format uses undefined type : {self.line}| {self.name} -> {self.type}"
        self.type = types[self.type]


class Signal:
    """Signal class for defining and verifying signals"""

    name: str = None
    format: Format = None
    scale: float = 1
    offset: float = 0
    range: Tuple[float, float] = None
    receiver: str = None
    line: int = None

    def __init__(
        self, line: int, name: str, format: Format, scale: float, offset: float, range: Tuple[float, float], receiver: str
    ) -> None:
        self.line = line
        self.name = str(name)
        self.format = format
        self.scale = float(scale)
        self.offset = float(offset)
        self.range = range
        self.receiver = str(receiver) if receiver else None

    def __str__(self) -> str:
        return f"{self.name} {str(self.format)} ({self.scale},{self.offset}) [{self.range[0]},{self.range[1]}] {self.receiver}"

    def verify(self, formats: Dict[str, Format], nodes: List[str]) -> None:
        """Verifies this Signal, matching it's format and receiver, should only be called once

        Args:
            formats (Dict[str, Format]): Defined Formats
            nodes (List[str]): List of all nodes
        """
        assert self.format in formats, f"Signal uses undefined format : {self.line}| {self.name} -> {self.format}"
        assert not self.receiver or self.receiver in nodes, f"Signal uses undefined node : {self.line}| {self.name} -> {self.receiver}"
        assert self.range[0] <= self.range[1], f"Signal range is invalid : {self.line}| {self.name} -> {self.range[0]} <\= {self.range[1]}"
        self.format = formats[self.format]


class Message:
    """Message class for defining and verifying messages"""

    id: int = 0
    name: str = None
    sender: str = None
    signals: List[Signal] = None
    line: int = None

    def __init__(self, line: int, id: int, name: str, sender: str) -> None:
        self.line = line
        self.id = int(id)
        self.name = str(name)
        self.sender = str(sender) if sender else None
        self.signals = []

    def __str__(self) -> str:
        sigs = listify([str(x) for x in self.signals], "\n    ")
        return f"{self.id} {self.name} {self.sender} :\n    {sigs}"

    def verify(self, nodes: List[str]) -> None:
        """Verifies this Message, matching it's sender, should only be called once

        Args:
            nodes (List[str]): List of all nodes
        """
        assert not self.sender or self.sender in nodes, f"Message uses undefined node : {self.line}| {self.name} -> {self.sender}"
        assert sum([sig.format.type.bits for sig in self.signals]) <= 64, f"Message exceeds 64 bits : {self.line}| {self.name}"


def parse(file: TextIO):
    """Parses a SDBC file given an open file

    Args:
        file (TextIO): The open file to parse
    """
    types: Dict[str, Type] = {}
    formats: Dict[str, Format] = {}
    messages: Dict[str, Message] = {}
    message_names: Set[str] = set()
    signals: Dict[str, Signal] = {}
    nodes = None
    version = None

    last_msg: Message = None

    line_no = 0
    try:
        for line in file.readlines():
            line_no += 1
            tks = tokenize(BytesIO(line.strip(" \n").encode("utf-8")).readline)
            next(tks)
            sel = next(tks)
            if sel.type == tkn.NAME:
                match sel.string:
                    case "VERSION":
                        last_msg = None
                        version = norm(next(tks))
                    case "NODES":
                        last_msg = None
                        next(tks)
                        nodes = [norm(t) for t in tks][:-2]
                    case "TYPE":
                        last_msg = None
                        typ = Type(line_no, norm(next(tks)), norm(next(tks)))
                        assert typ.name not in types, f"Type name already in use : {typ.line}| {typ.name}"
                        typ.verify()
                        types[typ.name] = typ
                    case "FORMAT":
                        last_msg = None
                        frmt = Format(
                            line_no,
                            norm(next(tks)),
                            next(tks).string == "-",
                            norm(next(tks)),
                            " ".join([norm(t) for t in tks][:-2]),
                        )
                        assert frmt.name not in formats, f"Format name already in use : {line_no}| {frmt.name}"
                        formats[frmt.name] = frmt
                    case "MSG":
                        id = norm(next(tks))
                        name = norm(next(tks))
                        assert next(tks).string == ":", f"Invalid signal syntax : {line_no}| {name}"
                        msg = Message(line_no, id, name, norm(next(tks)))
                        assert msg.id not in messages, f"Message ID already in use : {line_no}| {msg.name}:{msg.id}"
                        assert msg.name not in message_names, f"Message name already in use : {line_no}| {msg.id}:{msg.name}"
                        last_msg = msg
                        message_names.add(msg.name)
                        messages[msg.id] = msg
                    case "SIG":
                        name = norm(next(tks))
                        assert next(tks).string == ":", f"Invalid signal syntax : {line_no}| {name}"
                        format = norm(next(tks))
                        assert next(tks).string == "(", f"Invalid signal syntax : {line_no}| {name}"
                        scale = next_num(tks)
                        assert next(tks).string == ",", f"Invalid signal syntax : {line_no}| {name}"
                        offset = next_num(tks)
                        assert next(tks).string == ")", f"Invalid signal syntax : {line_no}| {name}"
                        assert next(tks).string == "[", f"Invalid signal syntax : {line_no}| {name}"
                        min = next_num(tks)
                        assert next(tks).string == ",", f"Invalid signal syntax : {line_no}| {name}"
                        max = next_num(tks)
                        assert next(tks).string == "]", f"Invalid signal syntax : {line_no}| {name}"
                        receiver = norm(next(tks))
                        sig = Signal(line_no, name, format, scale, offset, (min, max), receiver)
                        assert last_msg, f"Orphaned signal definition : {line_no}| {name}"
                        assert sig.name not in signals, f"Signal name already in use : {last_msg.name} -> {line_no}| {name}"

                        last_msg.signals.append(sig)
                        signals[sig.name] = sig

        for form in formats.values():
            form.verify(types)

        for sig in signals.values():
            sig.verify(formats, nodes)

        for msg in messages.values():
            msg.verify(nodes)
    except AssertionError as err:
        print(err)
        return

    print(f"SDBC {version}\n")

    print(listify(messages.values(), "\n\n"))


with open("Astriatus.sdbc", "r", encoding="utf-8") as db:
    parse(db)
