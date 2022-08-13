"""Script for parsing an SDBC file"""

from enum import Enum
from io import BytesIO
import io
from tokenize import TokenInfo, tokenize
import tokenize as tkn
from typing import Dict, Generator, List, Sequence, Set, TextIO


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
    tk: str = tk.string
    if tk.strip('"') != tk:
        return tk.strip('"')
    return tk.upper()


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
        if nxt and nxt.string == ":":
            next(self)
            return " ".join([t.string.strip('"') for t in self][:-2])
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


class PinType(Enum):
    ANALOG = "ANALOG"
    DIGITAL = "DIGITAL"


class IOType(Enum):
    O = "O"
    I = "I"


class Entry:
    """General class for all items in an SDBC file"""

    uid: int = 0
    name: str = None
    description: str = None
    line: int = None

    def __init__(self, uid: int, line: int, name: str, description: str) -> None:
        self.uid = int(uid)
        self.line = int(line)
        self.name = str(name)
        self.description = description


class Type(Entry):
    """Type class for defining and verifying types"""

    bits: int = None

    def __init__(self, uid: int, line: int, name: str, description: str, bits: int) -> None:
        super().__init__(uid, line, name, description)
        self.bits = int(bits)

    def __repr__(self) -> str:
        desc = f" | {self.description}" if self.description else ''
        name = f'\"{self.name}\"'
        return f"TYPE {name:8}{self.bits:4}{desc}\n"

    def __str__(self) -> str:
        return f"{self.name}@{self.bits}"

    def finalize(self) -> None:
        """Verifies this type"""
        assert self.bits <= 64, f"Type exceeds 64 bits : {self.line}| {self.name}:{self.bits}"
        assert self.bits > 0, f"Type has invalid number of bits : {self.line}| {self.name}:{self.bits}"


class Format(Entry):
    """Format class for defining and verifying formats"""

    signed: bool = False
    f_type: Type = None

    scale = 1
    offset = 0
    v_max = 0
    v_min = 0
    clamp = False  # Whether there is a min/max should be applied
    calc = False  # Whether scaling or shifting should be done

    def __init__(self, uid: int, line: int, name: str, description: str, signed: bool, f_type: Type, scale: float, offset: float, v_min: float, v_max: float) -> None:
        super().__init__(uid, line, name, description)
        self.signed = bool(signed)
        self.f_type = f_type
        self.offset = (v_max == self.v_max) and (v_min == self.v_min)
        self.calc = (scale == self.scale) and (offset == self.offset)
        self.scale = scale
        self.offset = offset
        self.v_max = v_max
        self.v_min = v_min

    def __repr__(self) -> str:
        desc = f" : {self.description}" if self.description else ''
        name = f'\"{self.name}\"'
        t_name = f'\"{self.f_type.name}\"'
        return f"FORMAT {name:8}{'-' if self.signed else '+'} {t_name:8} {tuple([self.scale, self.offset])} {[self.v_min, self.v_max]}{desc}\n"

    def __str__(self) -> str:
        return f"{self.name} {'signed' if self.signed else 'unsigned'} {str(self.f_type)}"

    def finalize(self, types: Dict[str, Type]) -> None:
        """Verifies this format, matching it's type, should only be called once

        Args:
            types (Dict[str, Type]): Defined Types
        """
        assert self.v_min <= self.v_max, f"Signal range is invalid : {self.line}| {self.name} -> {self.v_min} <\\= {self.v_max}"
        assert self.f_type in types, f"Format uses undefined type : {self.line}| {self.name} -> {self.f_type}"
        self.f_type = types[self.f_type]


class Signal(Entry):
    """Signal class for defining and verifying signals"""

    form: Format = None
    position: int = 0
    signed: bool = None
    bits: int = None
    link: Entry | str = None
    link_outgoing: bool = None

    def __init__(self, uid: int, line: int, name: str, description: str, form: Format, link: str, link_dir: IOType) -> None:
        super().__init__(uid, line, name, description)
        self.form = form
        self.link = link if link else None
        self.link_outgoing = True if link_dir is IOType.O else False

    def __repr__(self) -> str:
        desc = f" : {self.description}" if self.description else ''
        link = f" <- {self.link}" if self.link else ''
        f_name = f'\"{self.form.name}\"'
        return f"SIG {self.name:24} : {f_name:8}{link}{desc}\n"

    def __str__(self) -> str:
        return f"{self.name} {str(self.form)}"

    def finalize(self, c_node: Entry, formats: Dict[str, Format]) -> None:
        """Verifies this Signal, matching it's format and receiver, should only be called once

        Args:
            formats (Dict[str, Format]): Defined Formats
        """
        assert self.form in formats, f"Signal uses undefined format : {self.line}| {self.name} -> {self.form}"
        self.form = formats[self.form]
        self.bits = self.form.f_type.bits
        self.signed = self.form.signed
        if self.link:
            self.link = c_node[self.link]

    def set_pos(self, position: int):
        self.position = position


class Message(Entry):
    """Message class for defining and verifying messages"""

    c_node: Entry = None
    can_id: int = 0
    signals: List[Signal] = None

    def __init__(self, uid: int, line: int, name: str, description: str, c_node: Entry, can_id: int) -> None:
        super().__init__(uid, line, name, description)
        self.c_node = c_node
        self.can_id = int(can_id, 0)
        self.signals = []

    def __repr__(self) -> str:
        desc = f" : {self.description}" if self.description else ''
        fnl = f"MSG {self.can_id} {self.name}{desc}\n"
        for sig in self.signals:
            fnl += repr(sig)
        return fnl + '\n'

    def __str__(self) -> str:
        sigs = listify([str(x) for x in self.signals], "\n    ")
        blk = "|".join([str(x.form.f_type.bits) for x in self.signals])
        return f"{self.can_id} {self.name} :\n    {sigs}\n  |{blk}|"

    def finalize(self, formats: Dict[str, Format], nodes: Dict[str, Entry], g_pins: Set[int]) -> None:
        """Verifies this Message, matching it's sender, should only be called once

        Args:
            formats (Dict[str, Format]): Defined Formats
            nodes (List[str]): List of all nodes
        """

        shift = 64
        for sig in self.signals:
            sig.finalize(self.c_node, formats)
            shift -= sig.form.f_type.bits
            sig.set_pos(shift)

        assert sum([sig.form.f_type.bits for sig in self.signals]) <= 64, f"Message exceeds 64 bits : {self.line}| {self.name}"

        for node in nodes.values():
            if node is not self.c_node:
                assert self.can_id not in node.addr_space, f"Can id already in use in another address space {self.can_id} : {node.name}"


class Node(Entry):
    """Node class for defining nodes"""

    sources: dict[str, Entry] = None
    names: set[str] = None
    addr_space: set[int] = None

    def __init__(self, uid: int, line: int, name: str, description: str) -> None:
        super().__init__(uid, line, name, description)
        self.names = set()
        self.sources = {}
        self.addr_space = set()

    def add_address(self, addrs: Sequence | int) -> None:
        if isinstance(addrs, Sequence):
            for addr in addrs:
                self.addr_space.add(addr)
        else:
            self.addr_space.add(addrs)

    def __repr__(self) -> str:
        desc = f" : {self.description}" if self.description else ''
        fnl = f"NODE {self.name}{desc}\n"
        for sig in self.sources:
            fnl += repr(sig)
        return fnl + '\n'

    def __getitem__(self, item: str) -> Entry:
        assert isinstance(item, str), "Can only index with strings"
        return self.sources[item]

    def __str__(self) -> str:
        srcs = listify([str(x) for x in self.sources], "\n    ")
        return f"{self.name} :\n    {srcs}\n"

    def insert(self, __e: Entry) -> None:
        assert __e.name not in self.names, f"Source has already been defined in namespace {self.name} : {__e.name}"
        self.names.add(__e.name)
        self.sources[__e.name] = __e

    def finalize(self, formats: Dict[str, Format], nodes: Dict[str, Entry]) -> None:
        g_pins: Set[int] = set()
        srcs = [x for x in self.sources.values()]
        for src in srcs:
            if not isinstance(src, Signal):
                src.finalize(formats, nodes, g_pins)
        for k, v in [x for x in self.sources.items()]:
            if isinstance(v, Signal) or isinstance(v, SYNC):
                del self.sources[k]

    def mark(self):
        """Explicilty mark synced items that this node does not own"""
        for k, v in [x for x in self.sources.items()]:
            if v.c_node is not self:
                self.sources['_'+v.c_node.name+'_'+k] = v
                del self.sources[k]


class VIRT(Entry):
    """VIRT class for defining virtual pins"""

    c_node: Node = None
    cid: int = 0

    pin: int = None
    p_type = None

    def __init__(self, uid: int, line: int, name: str, description: str, c_node: Node, p_type: PinType) -> None:
        super().__init__(uid, line, name, description)
        self.c_node = c_node
        self.p_type = PinType(p_type)

    def __repr__(self) -> str:
        desc = f" : {self.description}" if self.description else ''
        fnl = f"VIRT {self.name} : {self.p_type.name}{desc}\n"
        return fnl

    def finalize(self, formats: Dict[str, Format], nodes: Dict[str, Entry], g_pins: Set[int]) -> None:
        self.pin = VIRT.cid + 1024
        VIRT.cid += 1
        assert self.pin not in g_pins, f"Virtual pin auto allocation failed : {self.pin}"
        g_pins.add(self.pin)


class GPIO(Entry):
    """GPIO class for defining physical pins"""

    c_node: Node = None

    pin: int = None
    p_type: PinType = None
    io_type: IOType = None

    def __init__(self, uid: int, line: int, name: str, description: str, c_node: Node, pin: int, p_type: PinType, io_type: IOType) -> None:
        super().__init__(uid, line, name, description)
        self.c_node = c_node
        self.pin = int(pin)
        self.p_type = PinType(p_type)
        self.io_type = IOType(io_type)

    def __repr__(self) -> str:
        desc = f" : {self.description}" if self.description else ''
        fnl = f"GPIO {self.name} : {self.pin} {self.p_type.name} {self.io_type.name}{desc}\n"
        return fnl

    def finalize(self, formats: Dict[str, Format], nodes: Dict[str, Entry], g_pins: Set[int]) -> None:
        assert self.pin not in g_pins
        g_pins.add(self.pin)


class SYNC(Entry):
    """SYNC class for defining sources to sync with"""

    c_node: Node = None
    o_node: Node | str = None
    o_name: str = None

    def __init__(self, uid: int, line: int, name: str, description: str, c_node: Node, o_node: str, o_name: str) -> None:
        super().__init__(uid, line, name, description)
        self.c_node = c_node
        self.o_node = o_node
        self.o_name = o_name

    def __repr__(self) -> str:
        desc = f" : {self.description}" if self.description else ''
        fnl = f"SYNC {self.o_node} : {self.o_name}{desc}\n"
        return fnl

    def finalize(self, formats: Dict[str, Format], nodes: Dict[str, Entry], g_pins: Set[int]) -> None:
        assert self.o_node is not self.c_node, "Attempting to sync with self"
        self.o_node = nodes[self.o_node]
        self.c_node.insert(self.o_node[self.o_name])
        for sig in self.o_node[self.o_name].signals:  # also insert signals of message
            self.c_node.insert(sig)


class SDBC:
    """Represents an SDBC file"""
    name: str = None
    version: str = None
    nodes: List[Node] = None
    types: List[Type] = None
    formats: List[Format] = None
    messages: List[Message] = None
    signals: List[Signal] = None

    def __init__(self, name: str, version: str, nodes: List[str], types: List[Type], formats: List[Format], messages: List[Message], signals: List[Signal]) -> None:
        self.name = name
        self.version = version
        self.nodes = nodes
        self.types = types
        self.formats = formats
        self.messages = messages
        self.signals = signals


class SDBC_m:
    """Represents an SDBC file"""
    version: str = None
    nodes: List[str] = None
    types: Dict[str, Type] = None
    formats: Dict[str, Format] = None
    messages: Dict[int, Message] = None
    message_names: Dict[str, Message] = None
    signals: Dict[str, Signal] = None

    def __init__(self, version: str, nodes: List[str], types: Dict[str, Type], formats: Dict[str, Format], messages: Dict[str, Message], message_names: Dict[str, Message], signals: Dict[str, Signal]) -> None:
        self.version = version
        self.nodes = nodes
        self.types = types
        self.formats = formats
        self.messages = messages
        self.message_names = message_names
        self.signals = signals


def parse(file: TextIO, mapped_result: bool = False) -> SDBC | SDBC_m:
    """Parses a SDBC file given an open file

    Args:
        file (TextIO): The open file to parse
    """
    file_name: str = None
    version: str = None

    types: Dict[str, Type] = {}
    formats: Dict[str, Format] = {}
    messages: Dict[int, Message] = {}
    signals: Dict[str, Signal] = {}
    nodes: Dict[str, Node] = {}

    last_node: Node = None
    last_msg: Message = None
    line_no = 0

    uid = 0

    try:
        for line in file.readlines():
            line_no += 1
            uid += 1
            tks = Peeker(tokenize(BytesIO(line.strip(" \n").encode("utf-8")).readline))
            next(tks)
            sel = next(tks)
            if sel.type == tkn.NAME:
                match sel.string:
                    case "NAME":
                        last_node = None
                        last_msg = None
                        file_name = tks.tkn()
                    case "VERSION":
                        last_node = None
                        last_msg = None
                        version = tks.tkn() + tks.tkn()
                    case "NODE":
                        last_msg = None
                        name = tks.tkn()
                        desc = tks.desc()
                        node = Node(uid, line_no, name, desc)
                        last_node = node
                        nodes[name] = node
                    case "GPIO":
                        last_msg = None
                        name = tks.tkn()
                        assert next(tks).string == ":", "Invalid signal syntax"
                        pin = tks.tkn()
                        p_type = tks.tkn()
                        io_type = tks.tkn()
                        desc = tks.desc()
                        gpio = GPIO(uid, line_no, name, desc, last_node, pin, p_type, io_type)
                        assert last_node, "No encapuslating node"
                        last_node.insert(gpio)
                    case "VIRT":
                        last_msg = None
                        name = tks.tkn()
                        assert next(tks).string == ":", "Invalid signal syntax"
                        p_type = tks.tkn()
                        desc = tks.desc()
                        virt = VIRT(uid, line_no, name, desc, last_node, p_type)
                        assert last_node, "No encapuslating node"
                        last_node.insert(virt)
                    case "SYNC":
                        last_msg = None
                        node = tks.tkn()
                        name = tks.tkn()
                        desc = tks.desc()
                        sync = SYNC(uid, line_no, '__SYNC:'+node+':'+name, desc, last_node, node, name)
                        assert last_node, "No encapuslating node"
                        last_node.insert(sync)
                    case "MSG":
                        can_id = tks.tkn()
                        name = tks.tkn()
                        desc = tks.desc()
                        msg = Message(uid, line_no, name, desc, last_node, can_id)
                        assert msg.can_id not in messages, f"Message CAN ID already in use : {msg.can_id}"
                        messages[msg.can_id] = msg
                        last_msg = msg
                        assert last_node, "No encapuslating node"
                        last_node.insert(msg)
                    case "SIG":
                        name = tks.tkn()
                        assert next(tks).string == ":", "Invalid signal syntax"
                        form = tks.tkn()
                        desc = tks.desc()
                        link = None
                        link_dir = IOType.O
                        if tks.peek and tks.peek.string == '<':
                            assert tks.tkn() == '<', "Invalid signal syntax"
                            assert tks.tkn() == '-', "Invalid signal syntax"
                            link = tks.tkn()
                        if tks.peek and tks.peek.string == '-':
                            assert tks.tkn() == '-', "Invalid signal syntax"
                            assert tks.tkn() == '>', "Invalid signal syntax"
                            link = tks.tkn()
                            link_dir = IOType.I
                        sig = Signal(uid, line_no, name, desc, form, link, link_dir)
                        assert last_msg, "Orphaned signal definition"
                        assert sig.name not in signals, f"Signal name already in use : {last_msg.name}"
                        last_msg.signals.append(sig)
                        signals[sig.name] = sig
                        assert last_node, "No encapuslating node"
                        last_node.insert(sig)
                    case "TYPE":
                        last_node = None
                        last_msg = None
                        name = tks.tkn()
                        bits = tks.tkn()
                        desc = tks.desc()
                        typ = Type(uid, line_no, name, desc, bits)
                        assert typ.name not in types, f"Type name already in use : {typ.name}"
                        typ.finalize()
                        types[typ.name] = typ
                    case "FORMAT":
                        last_node = None
                        last_msg = None
                        name = tks.tkn()
                        signed = next(tks).string == "-"
                        f_type = tks.tkn()
                        assert next(tks).string == "(", "Invalid signal syntax"
                        scale = tks.num()
                        assert next(tks).string == ",", "Invalid signal syntax"
                        offset = tks.num()
                        assert next(tks).string == ")", "Invalid signal syntax"
                        assert next(tks).string == "[", "Invalid signal syntax"
                        v_min = tks.num()
                        assert next(tks).string == ",", "Invalid signal syntax"
                        v_max = tks.num()
                        assert next(tks).string == "]", "Invalid signal syntax"
                        desc = tks.desc()
                        frmt = Format(uid, line_no, name, desc, signed, f_type, scale, offset, v_min, v_max)
                        assert frmt.name not in formats, f"Format name already in use : {line_no}| {frmt.name}"
                        formats[frmt.name] = frmt

        for form in formats.values():
            form.finalize(types)

        for node in nodes.values():
            node.finalize(formats, nodes)
            node.mark()

        assert len(messages) <= 1048575, "Exceeding maximum number of messages"

    except AssertionError as err:
        print(err, line_no)
        return

    print(f"{file_name} | SDBC : {version}")

    # if mapped_result:
    #     return SDBC_m(version, nodes, types, formats, messages, message_names, signals)

    return SDBC(file_name, version, nodes, list(types.values()), list(formats.values()), list(messages.values()), list(signals.values()))


def parse_file(filepath: str, mapped_result: bool = False) -> SDBC | SDBC_m:
    """Opens and parses an SDBC file

    Args:
        filepath (str): path to the file

    Returns:
        List[Message]: List of all the messages
    """
    with open(filepath, "r", encoding="utf-8") as sdbc:
        return parse(sdbc, mapped_result)


if __name__ == "__main__":
    parse_file("SDBC\Hawkrod.sdbc")
