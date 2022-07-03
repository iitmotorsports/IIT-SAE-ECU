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

    id: int = 0
    name: str = None
    bits: int = None
    line: int = None

    def __init__(self, line: int, id: int, name: str, bits: int) -> None:
        self.line = line
        self.id = int(id)
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

    id: int = 0
    name: str = None
    signed: bool = False
    type: Type = None
    comment: str = None
    line: int = None

    def __init__(self, line: int, id: int, name: str, signed: bool, type: Type, comment: str) -> None:
        self.line = line
        self.id = int(id)
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

    id: int = 0
    name: str = None
    format: Format = None
    scale: float = 1
    offset: float = 0
    range: Tuple[float, float] = None
    receiver: str = None
    line: int = None

    def __init__(
        self, line: int, id: int, name: str, format: Format, scale: float, offset: float, range: Tuple[float, float], receiver: str
    ) -> None:
        self.line = line
        self.id = int(id)
        self.name = str(name)
        self.format = format
        self.scale = float(scale)
        self.offset = float(offset)
        self.range = range
        self.receiver = str(receiver) if receiver else None

    def __str__(self) -> str:
        return f"{self.name} {str(self.format)} ({self.scale},{self.offset}) [{self.range[0]},{self.range[1]}] {self.receiver}"

    def __signal_id(self, size : int, shift : int, msg_id : int) -> int:
        return (size & 0x3F) << 26 | (shift & 0x3F) << 20 | msg_id & 0xFFFFF

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
    
    def set_id(self, msg_id : int, shift : int):
        self.id = self.__signal_id(self.format.type.bits, shift, msg_id)


class Message:
    """Message class for defining and verifying messages"""

    id: int = 0
    can_id: int = 0
    name: str = None
    sender: str = None
    signals: List[Signal] = None
    line: int = None

    def __init__(self, line: int, id: int, can_id: int, name: str, sender: str) -> None:
        self.line = line
        self.id = int(id)
        self.can_id = int(can_id)
        self.name = str(name)
        self.sender = str(sender) if sender else None
        self.signals = []

    def __str__(self) -> str:
        sigs = listify([str(x) for x in self.signals], "\n    ")
        blk = "|".join([str(x.format.type.bits) for x in self.signals])
        return f"{self.can_id} {self.name} {self.sender} :\n    {sigs}\n  |{blk}|"

    def verify(self, formats: Dict[str, Format], nodes: List[str]) -> None:
        """Verifies this Message, matching it's sender, should only be called once

        Args:
            formats (Dict[str, Format]): Defined Formats
            nodes (List[str]): List of all nodes
        """
        assert not self.sender or self.sender in nodes, f"Message uses undefined node : {self.line}| {self.name} -> {self.sender}"

        shift = 63
        for sig in self.signals:
            sig.verify(formats, nodes)
            shift -= sig.format.type.bits - 1
            sig.set_id(self.id, shift)

        assert sum([sig.format.type.bits for sig in self.signals]) <= 64, f"Message exceeds 64 bits : {self.line}| {self.name}"

class SDBC:
    """Represents an SDBC file"""
    types: List[Type] = None
    formats: List[Format] = None
    messages: List[Message] = None
    signals: List[Signal] = None

    def __init__(self, types: List[Type], formats: List[Format], messages: List[Message], signals: List[Signal]) -> None:
        self.types = types
        self.formats = formats
        self.messages = messages
        self.signals = signals

def parse(file: TextIO) -> SDBC:
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

    type_n = 0
    form_n = 0
    sign_n = 0
    msg_n = 0

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
                        typ = Type(line_no, type_n, norm(next(tks)), norm(next(tks)))
                        assert typ.name not in types, f"Type name already in use : {typ.line}| {typ.name}"
                        typ.verify()
                        type_n += 1
                        types[typ.name] = typ
                    case "FORMAT":
                        last_msg = None
                        frmt = Format(
                            line_no,
                            form_n,
                            norm(next(tks)),
                            next(tks).string == "-",
                            norm(next(tks)),
                            " ".join([norm(t) for t in tks][:-2]),
                        )
                        assert frmt.name not in formats, f"Format name already in use : {line_no}| {frmt.name}"
                        form_n += 1
                        formats[frmt.name] = frmt
                    case "MSG":
                        id = norm(next(tks))
                        name = norm(next(tks))
                        assert next(tks).string == ":", f"Invalid signal syntax : {line_no}| {name}"
                        msg = Message(line_no, msg_n, id, name, norm(next(tks)))
                        assert msg.can_id not in messages, f"Message ID already in use : {line_no}| {msg.name}:{msg.can_id}"
                        assert msg.name not in message_names, f"Message name already in use : {line_no}| {msg.can_id}:{msg.name}"
                        last_msg = msg
                        message_names.add(msg.name)
                        msg_n += 1
                        messages[msg.can_id] = msg
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
                        sig = Signal(line_no, sign_n, name, format, scale, offset, (min, max), receiver)
                        assert last_msg, f"Orphaned signal definition : {line_no}| {name}"
                        assert sig.name not in signals, f"Signal name already in use : {last_msg.name} -> {line_no}| {name}"
                        sign_n += 1
                        last_msg.signals.append(sig)
                        signals[sig.name] = sig

        for form in formats.values():
            form.verify(types)
            
        for msg in messages.values():
            msg.verify(formats, nodes)
            
        # assert len(types) <= 63, "Exceeding maximum number of types"
        # assert len(formats) <= 63, "Exceeding maximum number of formats"
        assert len(messages) <= 1048575, "Exceeding maximum number of messages"
        # assert len(signals) <= 8191, "Exceeding maximum number of signals"

    except AssertionError as err:
        print(err)
        return

    print(f"SDBC : {version}")

    messages = list(messages.values())

    print("\n", listify(messages, "\n\n"))

    return messages

def parse_file(filepath: str) -> SDBC:
    """Opens and parses an SDBC file

    Args:
        filepath (str): path to the file

    Returns:
        List[Message]: List of all the messages
    """
    with open(filepath, "r", encoding="utf-8") as sdbc:
        return parse(sdbc)

if __name__ == "__main__":
    parse_file("DBC/Astriatus.sdbc")
