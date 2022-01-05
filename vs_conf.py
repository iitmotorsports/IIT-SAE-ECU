""" Script to easily setup VScode """

import os
import sys
import glob
from io import TextIOBase
import json
import re
from typing import Any, Callable, Sequence

try:
    import serial
except ImportError:
    print("Attempting to install optional modules")
    os.system("python -m pip install serial")

try:
    import serial
except ImportError:
    print("Unable to install optional modules")

SETTINGS_PATH = ".vscode/settings.json"

BACKUP_SET = """{
    "FRONT_TEENSY_PORT": "COM3",
    "BACK_TEENSY_PORT": "COM10",
    "BAUDRATE": "115200",
    "GRAPH_ARG": "",
    "CORE_MODEL": "41",
    "CORE_NAME": "MK66FX1M0",
    "CORE_SPEED": "180000000",
    "USB_SETTING": "USB_SERIAL",
    "TOOLCHAIN_OFFSET": "../TeensyToolchain",
    "ADDITIONAL_CMAKE_VARS": "-DCUSTOM_BUILD_PATH_PREFIX:STRING=build/Pre_Build/",
    "CMAKE_FINAL_VARS": "-DENV_CORE_SPEED:STRING=${config:CORE_SPEED} -DENV_CORE_MODEL:STRING=${config:CORE_MODEL} -DENV_USB_SETTING:STRING=${config:USB_SETTING} ${config:ADDITIONAL_CMAKE_VARS}",
    "doxygen_runner.configuration_file_override": "${workspaceFolder}/docs/Doxyfile",
    "python.linting.pylintEnabled": true,
    "python.linting.enabled": true,
    "python.linting.flake8Enabled": false,
    "C_Cpp.clang_format_fallbackStyle": "{ BasedOnStyle: LLVM, UseTab: Never, IndentWidth: 4, TabWidth: 4, BreakBeforeBraces: Attach, AllowShortIfStatementsOnASingleLine: false, IndentCaseLabels: false, ColumnLimit: 0, AccessModifierOffset: -4 }",
    "search.exclude": {
        "**/build": true
    },
    "files.watcherExclude": {
        "**/build": true
    }
}"""


def comment_remover(text: str) -> str:
    """Remove C style comments from a str

    Args:
        text (str): str to remove comments from

    Returns:
        str: text with removed comments
    """

    def replacer(match):
        string = match.group(0)
        if string.startswith("/"):
            return ""
        return string.strip()

    pattern = r'//.*?$|/\*.*?\*/|\'(?:\\.|[^\\\'])*\'|"(?:\\.|[^\\"])*"'
    regex = re.compile(pattern, re.DOTALL | re.MULTILINE)
    regex = re.sub(r"\n\s*\n", "\n", re.sub(regex, replacer, text), re.MULTILINE)
    return regex.replace("},\n}", "}\n}")


def load_json() -> dict[str, str]:
    """Load the settings JSON

    Returns:
        dict[str, str]: the JSON as a dict
    """
    with open(SETTINGS_PATH, "r", encoding="UTF-8") as file:
        data = comment_remover(file.read())
        return json.loads(data)


def serial_ports() -> list[str]:
    """Lists serial port names

    Raises:
        EnvironmentError: On unsupported or unknown platforms

    Returns:
        list[str]: A list of the serial ports available on the system
    """
    if sys.platform.startswith("win"):
        ports = [f"COM{i + 1}" for i in range(256)]
    elif sys.platform.startswith("linux") or sys.platform.startswith("cygwin"):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob("/dev/tty[A-Za-z]*")
    elif sys.platform.startswith("darwin"):
        ports = glob.glob("/dev/tty.*")
    else:
        raise EnvironmentError("Unsupported platform")

    result = []
    for port in ports:
        try:
            serial_port = serial.Serial(port)
            serial_port.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    return result


def listify(seq: Sequence) -> str:
    """Convert sequence to a conventional string list

    Args:
        seq (Sequence): Sequence object

    Returns:
        str: the conventional string list
    """
    fnl = ""
    for option in seq:
        fnl += str(option) + ", "
    return fnl.strip(", ")


class Settings:
    """Wrapper for settings JSON"""

    class Option:
        """Key wrapper class"""

        key: str
        desc: str
        options: tuple
        advanced: bool
        setter: Callable[[str], str]
        getter: Callable[[str], str]

        value: str

        def __init__(
            self,
            key: str,
            desc: str,
            options: tuple = None,
            setter: Callable[[str], str] = str,
            getter: Callable[[str], str] = str,
            default: object = None,
            advanced: bool = False,
        ) -> None:
            self.key = key
            self.desc = desc
            self.options = tuple(str(x) for x in options) if options else None
            self.setter = setter
            self.getter = getter
            self.value = default
            self.advanced = advanced and default  # Advanced setting must have a default

        def __str__(self) -> str:
            out = f"{self.key} : {self.desc}"
            if self.advanced:
                out = "* " + out
            if self.value:
                out += f"\n    Default: {self.value}"
            if self.options:
                out += "\n    Options: "
                out += listify(self.options)
            return out

        def set_value(self, value: Any) -> bool:
            """Sets the value for this option

            Args:
                value (Any): The value to use to set

            Returns:
                bool: The value was successfully set
            """
            if not value and self.value:
                return True

            if self.options and str(value) not in self.options:
                return False

            self.value = self.setter(value)
            return True

        def get_value(self) -> str:
            """Get the value for this option

            Returns:
                str: The value of this option
            """
            return self.getter(self.value)

    FRONT_TEENSY_PORT = Option("FRONT_TEENSY_PORT", "Port representing the front ecu (COMx)", getter=lambda x: x.upper())
    BACK_TEENSY_PORT = Option("BACK_TEENSY_PORT", "Port representing the back ecu (COMx)", getter=lambda x: x.upper())
    GRAPH_ARG = Option(
        "GRAPH_ARG",
        "Enable graphical plotting of data",
        ("yes", "no"),
        lambda x: "yes" if x in ("-g", "yes") else "",
        lambda x: "-g" if x == "yes" else "",
        default="no",
    )
    CORE_MODEL = Option("CORE_MODEL", "Model number of the teensy to compile for", (36, 40, 41), default=41)
    CORE_SPEED = Option("CORE_SPEED", "Speed at which the CPU will run (MHz)", default="AUTOSET VALUE", advanced=True)
    CORE_NAME = Option("CORE_NAME", "Model of the cpu", default="AUTOSET VALUE", advanced=True)
    BAUDRATE = Option("BAUDRATE", "Baudrate to use with serial", (9600, 19200, 38400, 57600, 115200), default=115200, advanced=True)
    USB_SETTING = Option("USB_SETTING", "USB behavior of the core", default="USB_SERIAL", advanced=True)
    TOOLCHAIN_OFFSET = Option("TOOLCHAIN_OFFSET", "Offset to the toolchain", default="../TeensyToolchain", advanced=True)
    ADDITIONAL_CMAKE_VARS = Option(
        "ADDITIONAL_CMAKE_VARS", "More defines passed to CMake", default="-DCUSTOM_BUILD_PATH_PREFIX:STRING=build/Pre_Build/", advanced=True
    )

    options = (
        FRONT_TEENSY_PORT,
        BACK_TEENSY_PORT,
        GRAPH_ARG,
        CORE_MODEL,
        CORE_SPEED,
        CORE_NAME,
        BAUDRATE,
        USB_SETTING,
        TOOLCHAIN_OFFSET,
        ADDITIONAL_CMAKE_VARS,
    )

    settings: dict[str, str]

    def __init__(self, settings_dict: dict[str, str]) -> None:
        self.load(settings_dict)
        self.CORE_SPEED.getter = self.__get_core_speed
        self.CORE_NAME.getter = self.__get_core_name

    def __get_core_speed(self, _) -> str:
        model = int(self.CORE_MODEL.value)
        if model == 36:
            return "180000000"
        if model == 40 or model == 41:
            return "600000000"
        return "BAD MODEL"

    def __get_core_name(self, _) -> str:
        model = int(self.CORE_MODEL.value)
        if model == 36:
            return "MK66FX1M0"
        if model == 40 or model == 41:
            return "IMXRT1062"
        return "BAD MODEL"

    def load(self, settings_dict: dict[str, str]) -> None:
        """Loads settings dict

        Args:
            settings_dict (dict[str, str]): settings dict

        Raises:
            KeyError: Key is not found in the settings JSON
        """
        self.settings = settings_dict
        for option in self.options:
            if option.key not in self.settings:
                raise KeyError(f"Key not found in settings JSON: {option.value}")
            else:
                option.set_value(self.settings[option.key])

    def print_settings(self) -> None:
        """Print the current settings"""
        for option in self.options:
            print(option.key, option.value)

    def unload(self, file: TextIOBase) -> None:
        """Write settings to file

        Args:
            file (TextIOBase): The file to write to
        """
        for option in self.options:
            self.settings[option.key] = option.get_value()
        json.dump(self.settings, file, indent=4)


def main():
    """Main function"""

    vs_code_startup = len(sys.argv) == 2 and sys.argv[1] == "thisisbeingrunonstartup"
    adv_mode = False
    first_time = not os.path.exists(SETTINGS_PATH)

    if first_time:
        with open(SETTINGS_PATH, "w", encoding="UTF-8") as sett:
            sett.write(BACKUP_SET)
    elif vs_code_startup:
        settings = Settings(load_json())
        print(f"Configured for Teensy{settings.CORE_MODEL.get_value()} @ {int(int(settings.CORE_SPEED.get_value())/1000000)} Mhz")
        print(f"Current ports:\n Front:\t{settings.FRONT_TEENSY_PORT.get_value()}\n Back:\t{settings.BACK_TEENSY_PORT.get_value()}")
        sys.exit(0)

    settings = Settings(load_json())

    for option in settings.options:
        if option.advanced and not adv_mode:
            if first_time:
                break
            if input("Enter 'Yes' to edit advanced options: ") != "Yes":
                break
            adv_mode = True
        print(option)
        if option is settings.FRONT_TEENSY_PORT or option is settings.BACK_TEENSY_PORT:
            print("    Open serial ports:", listify(serial_ports()))
        if not option.set_value(input("Input option, blank for default: ")):
            while not option.set_value(input("Invalid option: ")):
                pass

    with open(SETTINGS_PATH, "w", encoding="UTF-8") as sett:
        settings.unload(sett)


main()
