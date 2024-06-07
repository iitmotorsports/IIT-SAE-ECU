# DBC Logging

## Goals

- Better standardize how telemetry values are sent and decoded
- More static mapping, reduce the necessity to extract known information
- Separate telemetry from logging

## Distinction

### Telemetry

Includes things such as measurements, systems states, information not immediately useful to a person.
Because, ultimately, we will have a set number of measurements that are taken, these mappings can be more static.
In other words, these don't change often.
Could be used for M2M.

### Logging

More of an event log, pushing when something is happening on either ECU.
Should retain previous logging protocol, as it is not static, programmer should be able to log anywhere.
Should not be used for M2M.

## Flow of data

The following section only refers to telemetry.

1. Manually (or automatically?) create mappings of known and wanted values, including, data type, format, etc.
2. Generate all defines/IDs in a header file that represent each value given the mapping
3. Given the appropriate ID, the underlying logic for telemetry is able to process a value based on it's type, format, etc.
4. Decoding uses that same mapping, being able to more easily format and represent a value.

## Requirements

Requirements that should be set, in order to improve functionality

### Message size

Currently 8 Bytes. However, this does not need to be the case when saving a value to storage, or sending it over USB Serial.
Over CAN, however, it must remain at 8 bytes, unless we actually use CAN addresses to differentiate between messages.
This would mean more attention to what addresses we are using, but it would allow for more data to be sent at once over CAN.
Doing so could free up the 4 bytes used for ID.

### CAN Nodes

There are other nodes besides the ECUs and, potentially, even more nodes for future builds. Being able to get information directly from these nodes instead of having it pass through an ECU first would be beneficial. Additionally, it would help differentiate where messages are actually coming from, instead of "its from the ecu that takes it from the mc" or whatever other nodes.

This does mean that messages from other nodes must be mapped.

## Solutions

### Spreadsheet mapping

Each value is mapped out with their own variable name, type, unit, etc. Each type and unit is also defined within the same book.
Data validation enables easier entry of values. How exactly values are "packaged" and passed over CAN is decided by grouping as many values into messages based on their value size (bits). This means another mapping is generated, indicating where and how to decode for any particular value.

#### Pros

Using a spreadsheet, this enables for easier entry of values, as data validation helps ensure we use the defined types and units.

Not as "scary" as the other options, allows us to stick notes everywhere for new members

#### Cons

Requires use of another generated mapping

Other sub systems are not accounted for, such as MCs and AMS. Their values would have to pass through an ECU before being recorded.

Being an excel file, this requires installing additional packages to run through a script.

### Custom mapping

Similar to the spreadsheet mapping, but does not use a spreadsheet but a custom format to store the same information.

#### Pros

No special packages needed

Simple, just a text file that anyone can edit with a set syntax

#### Cons

Parser must be made wherever this is needed

Still requires use of an additional mapping of where data actually is placed

subsystems unaccounted for

### Vector DBC

Use a more industry standard method of organizing can messages

This standard organizes values into messages and signals, where messages are made up of signals and signals dictate things such as can ID, bit size, position, types, etc. specific to the standard.

Just a passive mapping, actual transmission of data is still done normally

#### Pros

Standard, there are tools and libraries that are specific to this format. As such, a tool exists to help generate these mappings

Would account for other subsystems

static organization means no extra mappings should be generated

Has support to multiplex messages; signals in a message change depending on an indicator values within the message

#### Cons

Values must be explicitly organized, with addresses, bit position and length. It can potentially get very involved just to change a single value.

attributes of a signal are limited to the standard, custom attributes can be made but it can complicate decoding if it has to be done manually

most libraries found for decoding and parsing are in python or c/c++

### Custom DBC

Follow a similar structure to the Vector DBC but modified to fit our needs.

the dbc file is structured in a way that is only relevant to us.
One example is, instead of stating exactly the number of bits and position of a signal within a message, we just define a signal by variable and format and which messages they should go in.
Formats themselves would define their types, bit size, scale and offset, etc.
The signals would just go in the order that they are defined in a message.
Signals could also be defined separately from messages?
All messages are 8 bytes and little endian

We could potentially convert it to vector dbc to allow use of other tools

#### Pros

Less confusion with the format and, potentially, decoding of values.

Could improve readability of the raw dbc files for anyone to edit directly

Could potentially use the spreadsheet to initially make the mapping and then generate the sdbc file.

#### Cons

All the tools and libraries for vector dbc will not work, as we are simply following it conceptually

We would have to validate message definitions, ensuring it all makes sense. Has to be done anyways, but we need to make sure of it as there are libraries to do that for us.

defining multiplexed messages might be confusing
