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


class Peeker:

    generator: Generator = None
    peek: TokenInfo = None
    empty = False

    def __init__(self, generator):
        self.generator = generator
        try:
            self.peek = next(self.generator)
        except StopIteration:
            self.empty = True

    def __iter__(self):
        return self

    def __next__(self):
        """
        Return the self.peek element, or raise StopIteration
        if empty
        """
        if self.empty:
            raise StopIteration()
        to_return = self.peek
        try:
            self.peek = next(self.generator)
        except StopIteration:
            self.peek = None
            self.empty = True
        return to_return

    def tkn(self) -> str:
        """Returns the next formatted token

        Returns:
            str: next token
        """
        return norm(next(self))

    def num(self) -> float:
        """Gets the next number, uses up to two tokens

        Returns:
            float: number as a float, error if not found
        """
        frst = next(self).string

        if frst == "-" or frst == "+":
            frst = frst + next(self).string

        return float(frst)

    def desc(self) -> str | None:
        """Gets the description, provided it exists, uses up as many tokens that remain

        Returns:
            str | None: The description as a string, or None if it does not exist
        """
        nxt = self.peek
        if nxt and nxt.string == "|":
            next(self)
            return " ".join([norm(t) for t in self][:-2])
        return None

    def node(self) -> str | None:
        """Gets the name of a node, provided it exists, uses one token

        Returns:
            str | None: The node as a string, or None if it does not exist
        """
        nxt = self.peek
        if nxt.string == "|":
            return None
        return norm(next(self))


class Entry:
    """General class for all items in an SDBC file"""

    uid: int = 0
    name: str = None
    description: str = None
    line: int = None

    def __init__(self, uid: int, name: str, description: str, line: int) -> None:
        self.uid = int(uid)
        self.line = int(line)
        self.name = str(name)
        self.description = description


class Type(Entry):
    """Type class for defining and verifying types"""

    bits: int = None

    def __init__(self, uid: int, name: str, description: str, line: int, bits: int) -> None:
        super().__init__(uid, name, description, line)
        self.bits = int(bits)

    def __str__(self) -> str:
        return f"{self.name}@{self.bits}"

    def verify(self) -> None:
        """Verifies this type"""
        assert self.bits <= 64, f"Type exceeds 64 bits : {self.line}| {self.name}:{self.bits}"
        assert self.bits > 0, f"Type has invalid number of bits : {self.line}| {self.name}:{self.bits}"


class Format(Entry):
    """Format class for defining and verifying formats"""

    signed: bool = False
    f_type: Type = None

    def __init__(self, uid: int, name: str, description: str, line: int, f_type: Type, signed: bool) -> None:
        super().__init__(uid, name, description, line)
        self.signed = bool(signed)
        self.f_type = f_type

    def __str__(self) -> str:
        return f"{self.name} {'signed' if self.signed else 'unsigned'} {str(self.f_type)}"

    def verify(self, types: Dict[str, Type]) -> None:
        """Verifies this format, matching it's type, should only be called once

        Args:
            types (Dict[str, Type]): Defined Types
        """
        assert self.f_type in types, f"Format uses undefined type : {self.line}| {self.name} -> {self.f_type}"
        self.f_type = types[self.f_type]


class Signal(Entry):
    """Signal class for defining and verifying signals"""

    form: Format = None
    scale: float = 1
    offset: float = 0
    minmax: Tuple[float, float] = None
    receiver: str = None

    position: int = 0
    signed: bool = None
    bits: int = None

    def __init__(self, uid: int, name: str, description: str, line: int, form: Format, scale: float, offset: float, minmax: Tuple[float, float], receiver: str) -> None:
        super().__init__(uid, name, description, line)
        self.form = form
        self.scale = float(scale)
        self.offset = float(offset)
        self.minmax = minmax
        self.receiver = str(receiver) if receiver else None

    def __str__(self) -> str:
        return f"{self.name} {str(self.form)} ({self.scale},{self.offset}) [{self.minmax[0]},{self.minmax[1]}] {self.receiver}"

    def verify(self, formats: Dict[str, Format], nodes: List[str]) -> None:
        """Verifies this Signal, matching it's format and receiver, should only be called once

        Args:
            formats (Dict[str, Format]): Defined Formats
            nodes (List[str]): List of all nodes
        """
        assert self.form in formats, f"Signal uses undefined format : {self.line}| {self.name} -> {self.form}"
        assert not self.receiver or self.receiver in nodes, f"Signal uses undefined node : {self.line}| {self.name} -> {self.receiver}"
        assert self.minmax[0] <= self.minmax[
            1], f"Signal range is invalid : {self.line}| {self.name} -> {self.minmax[0]} <\= {self.minmax[1]}"
        self.form = formats[self.form]
        self.bits = self.form.f_type.bits
        self.signed = self.form.signed

    def set_pos(self, position: int):
        self.position = position


class Message(Entry):
    """Message class for defining and verifying messages"""

    can_id: int = 0
    sender: str = None
    signals: List[Signal] = None

    def __init__(self, uid: int, name: str, description: str, line: int, can_id: int, sender: str) -> None:
        super().__init__(uid, name, description, line)
        self.can_id = int(can_id)
        self.sender = str(sender) if sender else None
        self.signals = []

    def __str__(self) -> str:
        sigs = listify([str(x) for x in self.signals], "\n    ")
        blk = "|".join([str(x.form.f_type.bits) for x in self.signals])
        return f"{self.can_id} {self.name} {self.sender} :\n    {sigs}\n  |{blk}|"

    def verify(self, formats: Dict[str, Format], nodes: List[str]) -> None:
        """Verifies this Message, matching it's sender, should only be called once

        Args:
            formats (Dict[str, Format]): Defined Formats
            nodes (List[str]): List of all nodes
        """
        assert not self.sender or self.sender in nodes, f"Message uses undefined node : {self.line}| {self.name} -> {self.sender}"

        shift = 64
        for sig in self.signals:
            sig.verify(formats, nodes)
            shift -= sig.form.f_type.bits
            sig.set_pos(shift)

        assert sum([sig.form.f_type.bits for sig in self.signals]) <= 64, f"Message exceeds 64 bits : {self.line}| {self.name}"


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
            tks = Peeker(
                tokenize(BytesIO(line.strip(" \n").encode("utf-8")).readline))
            next(tks)
            sel = next(tks)
            if sel.type == tkn.NAME:
                match sel.string:
                    case "VERSION":
                        last_msg = None
                        version = tks.tkn()
                    case "NODES":
                        last_msg = None
                        next(tks)
                        nodes = [norm(t) for t in tks][:-2]
                    case "TYPE":
                        last_msg = None
                        name = tks.tkn()
                        bits = tks.tkn()
                        desc = tks.desc()
                        typ = Type(type_n, name, desc, line_no, bits)
                        assert typ.name not in types, f"Type name already in use : {typ.line}| {typ.name}"
                        typ.verify()
                        type_n += 1
                        types[typ.name] = typ
                    case "FORMAT":
                        last_msg = None
                        name = tks.tkn()
                        signed = next(tks).string == "-"
                        f_type = tks.tkn()
                        desc = tks.desc()
                        frmt = Format(form_n, name, desc, line_no, f_type, signed)
                        assert frmt.name not in formats, f"Format name already in use : {line_no}| {frmt.name}"
                        form_n += 1
                        formats[frmt.name] = frmt
                    case "MSG":
                        can_id = tks.tkn()
                        name = tks.tkn()
                        assert next(
                            tks).string == ":", f"Invalid signal syntax : {line_no}| {name}"
                        sender = tks.node()
                        desc = tks.desc()
                        msg = Message(msg_n, name, desc, line_no, can_id, sender)
                        assert msg.can_id not in messages, f"Message ID already in use : {line_no}| {msg.name}:{msg.can_id}"
                        assert msg.name not in message_names, f"Message name already in use : {line_no}| {msg.can_id}:{msg.name}"
                        last_msg = msg
                        message_names.add(msg.name)
                        msg_n += 1
                        messages[msg.can_id] = msg
                    case "SIG":
                        name = tks.tkn()
                        assert next(
                            tks).string == ":", f"Invalid signal syntax : {line_no}| {name}"
                        form = tks.tkn()
                        assert next(
                            tks).string == "(", f"Invalid signal syntax : {line_no}| {name}"
                        scale = tks.num()
                        assert next(
                            tks).string == ",", f"Invalid signal syntax : {line_no}| {name}"
                        offset = tks.num()
                        assert next(
                            tks).string == ")", f"Invalid signal syntax : {line_no}| {name}"
                        assert next(
                            tks).string == "[", f"Invalid signal syntax : {line_no}| {name}"
                        min = tks.num()
                        assert next(
                            tks).string == ",", f"Invalid signal syntax : {line_no}| {name}"
                        max = tks.num()
                        assert next(
                            tks).string == "]", f"Invalid signal syntax : {line_no}| {name}"

                        receiver = tks.node()
                        desc = tks.desc()

                        sig = Signal(sign_n, name, desc, line_no, form, scale, offset, (min, max), receiver)
                        assert last_msg, f"Orphaned signal definition : {line_no}| {name}"
                        assert sig.name not in signals, f"Signal name already in use : {last_msg.name} -> {line_no}| {name}"
                        sign_n += 1
                        last_msg.signals.append(sig)
                        signals[sig.name] = sig

        for form in formats.values():
            form.verify(types)

        for msg in messages.values():
            msg.verify(formats, nodes)

        assert len(messages) <= 1048575, "Exceeding maximum number of messages"

    except AssertionError as err:
        print(err)
        return

    print(f"SDBC : {version}")

    messages = list(messages.values())

    print("\n", listify(messages, "\n\n"))

    return SDBC(list(types.values()), list(formats.values()), messages, list(signals.values()))


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
