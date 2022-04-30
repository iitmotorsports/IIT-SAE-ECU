"""Regex definitions for logging"""

STATE_PASS = r"\" *(\S+.*?) +(?i:state)\s*\""  # TODO: check for state in TAG or SXX check

SPECIAL_PASS = (
    r"_LogPrebuildString\s*\(\s*(\".*?\")\s*\)"  # _LogPrebuildString("Str") # Special case where we can indirectly allocate a string
)
SPECIAL_FAIL = r"_LogPrebuildString\s*\(\s*([^\"]*?)\s*\)"  # TODO: finish coverage on fail regex

TAG_PASS = r"LOG_TAG(?= )\s*[^\"=]+?=\s*(\"\s*\S.*\")\s*;"  # LOG_TAG idVar = "ID"; # Where ID cannot be blank
TAG_FAIL = r"(?:LOG_TAG(?= )\s*[^\"=]+?=\s*)(?:[^\"=]+?|\"\s*\")\s*;"  # Implicit, single char, or empty string definition of a tag type

CALL_SS = r"(?:Log\s*\.*\s*([diwef])?\s*\(\s*(\"\s*\S(?:\\.|[^\"])+\")\s*,\s*(\"(\\.|[^\"])*\")\s*\)\s*;)"  # -> Log("Str", "Str");
CALL_VS = r"(?:Log\s*\.*\s*([diwef])?\s*\(\s*([^\"]+?)\s*,\s*(\"(?:\\.|[^\"])*\")\s*\)\s*;)"  # -> Log(Var, "Str");
CALL_SSV = (
    r"(?:Log\s*\.*\s*([diwef])?\s*\(\s*(\"\s*\S(?:\\.|[^\"])+\")\s*,\s*(\".*?\")\s*,\s*(.+?)\s*\)\s*;)"  # -> Log("Str", "Str", Var);
)
CALL_VSV = r"(?:Log\s*\.*\s*([diwef])?\s*\(\s*([^\"]+?)\s*,\s*(\".*?\")\s*,\s*(.+?)\s*\)\s*;)"  # -> Log(Var, "Str", Var);

CALL_ERR_LITERAL = r"(?:(Log\s*\.*\s*(?:[diwef])?\s*\(\s*(?:[^\"]+?|\"(?:[^\"]|\\\")*?\")\s*,\s*)([^\";]+?)(\s*(?:,\s*(?:.+?))?\s*\)\s*;))"  # Message string is not a literal string | IDE will warn about numbers but will still compile
CALL_ERR_BLANK = r"(?:(Log\s*\.*\s*(?:[diwefp])?\s*\(\s*)(\"\s*\")(\s*,.*?\)\s*;))"  # Blank string ID

CALL_P = r"(?:Log\s*\.*\s*(p)\s*\(\s*([^\",]*?|\".*?\")\s*,\s*(.*?)\s*,\s*(\".*?\")\s*,\s*([^,]*?)\s*(?:,\s*{\s*(.*?)\s*}\s*(?:,\s*(.*?)\s*)?)?\)\s*;)"  # -> Log.p(Var, "Str", "Str", Var); || Log.p("Var", "Str", "Str", Var, {"Str", ...}, Var);
CALL_ERR_P_LITERAL = r"(?:(Log\s*\.*\s*p\s*\(\s*(?:[^\"]+?|\"(?:[^\"]|\\\")*?\")\s*,\s*(?:[^\",]*?|\".*?\")\s*,\s*)([^\";]+?)(\s*(?:,\s*(?:.+?))?\s*\)\s*;))"  # CALL_ERR_LITERAL for posts

# (?:(Log\s*\.*\s*p\s*\(\s*(?:[^\"]+?|\"(?:[^\"]|\\\")*?\")\s*,\s*(?:[^\",]*?|\".*?\")\s*,\s*)([^\";]+?)(\s*(?:,\s*(?:.+?))?\s*\)\s*;))
# Log\s*.\s*p\s*\(\s*([^\",]*?|\".*?\")\s*,\s*([^\",]*?|\".*?\")\s*,\s*([^\s\"{}].*?)\s*,\s*([^,{}]*?)\s*(?:,\s*{\s*(.*?)\s*}\s*(?:,\s*(.*?)\s*)?)?\)\s*;