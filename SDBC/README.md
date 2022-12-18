# SDBC syntax

Version 1.0.0

## Global syntax

- First token on a line indicates what syntax to follow
- Any text after a final `:` in a line is considered the description of that line. If there is no final `:` there is no description
- IDs must be unique to their namespace
  - Namespaces are by level, for example, `NODE`s have unique IDs and `MSG`s within a single `NODE` have unique IDs
- Indents are not necessary but are recommended to format the document appropriately
- The order in which lines are defined is important
- IDs shall only consist of uppercase letters, underscores, and numbers where a number is not the first character

## Top level lines

- `NAME`
- `VERSION`
- `CONF`
- `NODE`
- `FORMAT`
- `TYPE`

## Name

> NAME "name here"

This shall be the first line in the file. It indicates the name of the SDBC file. It shall match the name of the file, omitting the extension.

## Version

> VERSION 1.0.0

This shall be the second line of the file. It indicates what version of the SDBC standard it follows.
Semantic versioning shall be used with no extensions.

## Configuration

> CONF
> > ANALOG_RESOLUTION : `Integer`

The `CONF` line denotes configuration values that the system should abide to. It is up to the device implementation to ensure these configurations are followed or expected.

**All** configuration values must be set per the specification version.

### Configuration values

- `ANALOG_RESOLUTION`
  - The analog bit resolution of GPIO pins
  - e.g. 12 bit resolution results in a max value of 4096 for analog values and a datatype of `int16_t`

## Nodes

> NODE `node_id` : `optional description`
> > MAX_GPIO : `max_gpio_number` : `optional description`
> > VIRT_ID_OFF : `virtual_id_offset` : `optional description`
> > ADDR `can_address_low` - `can_address_high` : `optional description`
> > GPIO `gpio_pin_id` : `pin#` `[DIGITAL|ANALOG]` `[I|O]` : `optional description`  
> > VIRT `virt_pin_id` : `[DIGITAL|ANALOG]` : `optional description`  
> > MSG `can_msg_address`  `[<<|>>]` `can_msg_id` : `optional description`  
> > > SIG `signal_id` : `"format_name"` `[<<|>>]` `[virt_pin_id|gpio_pin_id]` : `optional description`  
> > LINK `[<<|>>]` `node_id` `can_msg_id`: `optional description`  
> > > SET `signal_id` `[<<|>>]` `[virt_pin_id|gpio_pin_id]` : `optional description`  

Where `pin#` is an integer.

Nodes are used to organize where CAN messages originate from and to define pins that are in use. Defining pins is not necessary, such as with a motor controller or AMS, where there is no embedded logic. Nodes must be part of the CAN network. SDBC assumes that there is only one network.

### `NODE`

The `NODE` line consists of a unique ID for this node, and the optional description. It is important that the name matches that of the build system's if the node is related to the pins defined i.e. the ECUs.

### Metadata

#### `MAX_GPIO`

The `MAX_GPIO` line defined the maximum number of GPIO pins that a `NODE` is capable of addressing, regardless of whether it is declared or not. This allows nodes to, for whatever reason, still access gpio pins not defined by SDBC. Limiting this range helps prevent undefined behavior and mistakes when testing. This line **must** be set when any `GPIO` is defined on a `NODE`.

This number can often be found by looking into the appropriate GPIO headers for a particular node's processor.

#### `VIRT_ID_OFF`

The offset that virtual pin numbers should start from. Because `VIRT`s should not collide with `GPIO` pin numbers, their pseudo pin number must start with an offset that clears any `GPIO` pin number.

This line **must** be set when any `VIRT` is defined on a `NODE`.

#### `ADDR`

The `ADDR` line optionally defines a CAN bus address space that should be allocated to this node. This means no other nodes can use any addresses within this space. Any sources within this node that use an address outside of this range should not cause error but should show a warning and be added to the address space, where applicable. It shall not be allowed for any source or address space to overlap in address.

The first address is the lower bound while the second is the upper bound.

If no address space is defined, only the refrenced addresses in the node are used.

Addresses can either be in hex (begins with `0x`) or decimal.

Multiple `ADDR` may be defined, however, a warning shall be issued if definitions overlap within the same `NODE`.

### Sources

#### `GPIO`

The `GPIO` line defines a physical GPIO pin that shall be in use. It consists of a unique ID, the pin number, whether this pin should be treated as `DIGITAL` or `ANALOG`; which changes whether it should be treated as a single bit or multiple, whether this pin is exclusively for input (`I`) or output (`O`), and the optional description.

In this context, input means that the physical pin can only be read and output means that the physical pin can only be set. However, both input and output pins will store a buffer value that can be read, this allows to keep track of both their states.

**NOTE:** `GPIO`s can only be set to, at most, a 32 bit integer pin will not be verified if it is necessarily usable as digital or analog, refer to board pin-out to confirm it has digital, analog read and/or write capabilities.

#### `VIRT`

The `VIRT` line defines a virtual pin that is mainly used to communicate M2M but treated as a pin to simplify the API. It consists of a unique ID, whether this pin should be treated as `DIGITAL` or `ANALOG`; which changes whether it should be treated as a single bit or multiple, and the optional description.

Virtual pins shall be defined on the node of the initial value or whichever node generates the value, as it can technically be defined anywhere in the network.

In this context, input means that the value can only be read and output means that the value can only be set.

Because there is no pin given, the usage of this pin must verify it has a unique or alternative way to modify the value compared to the set GPIO lines.

**NOTE:** `VIRT`s can only be set to, at most, a 64 bit floating point value.

`GPIO`s and `VIRT`s are functionally the same. Unless otherwise noted, `VIRT`s can used where `GPIO` can and vice versa. The rest of this specification will mainly refer to `GPIO`.

#### `MSG`

The `MSG` line defines a CAN bus message, where it contains `SIG`s or signals, which are the values within that message. It consists of the can bus address for the message, whether this message is outgoing `<<` or incoming `>>`, the unique id for this message, and the optional description.

Addresses can either be in hex (begins with `0x`) or decimal.

There are can be multiple signals per message, however, it shall be verified at processing whether the sigs are able to fit in the message's 64 bit buffer. The order in which sigs are defined in a message dictates the structure of the buffer. i.e. if a message is defined with an `int`, and two `short`s, in that order, the actual buffer will be sent where the first 32 bits are an `int` and the next two 16 bit blocks are `short`s, where it all totals to 64 bits.

##### `SIG`

The `SIG` line consists of the signal id, the format of this signal, optionally, the id of the value that should be linked to this signal using a value setter `<<` or `>>`, dependent on whether this message is outgoing or incoming, respectively; this keeps the signal up to date with the given source whether it is from a gpio or virt source, and the optional description.

Signals linked with a value setter (`<<` or `>>`) are to be updated in the background at runtime. The header for formatting the incoming values will be generated but the actual logic must be then later defined in a separate source file.

**NOTE:** `GPIO`s can only be set to, at most, a 32 bit integer value while `VIRT`s are doubles.

##### Space optimization

Although not required, effort should be made to optimize the placement of signals within a message.

This can be checked for at parsing.

Values should be optimized to be placed at appropriate integer intervals, where possible. i.e. every 32 bits for 32 bit values, every 16 bits for 16 bit values, or every 8 bits for 8 bit values. This is best achieved by placing values largest to smallest, where applicable.

#### `LINK`

The `LINK` line defines a `GPIO` link between nodes, where it contains `SET`s or setters, which dictate what value gets set to what. It consists of whether this link is outgoing `<<` or incoming `>>`, the unique ID for the linked node, the unique ID for the message on the linked node, and the optional description.

Incoming `>>` `LINK`s set values on the node that defines it. Outgoing `<<` `LINK`s set values on the node that it is linking to.

**NOTE:** For each node, only one `LINK` can be defined per message.

##### `SET`

The `SET` line consists of a signal ID from the linked node's message, `<<` or `>>`, dependent on whether this `LINK` is outgoing or incoming, respectively; this keeps the signal up to date with the given source whether it is from a `GPIO` or `VIRT`, and the optional description.

**NOTE: ALL** values of a linked `NODE`s `MSG` must be referenced for both incoming and outgoing messages.

#### `MSG` `SIG` Value Setter V.S. `LINK`s

Value setters and `LINK`s work very similarly, however, value setters are general and restricted in the sense that it is attached only to the address, which cannot be explicitly referenced by other nodes. A `MSG` is either outgoing, with a node `GPIO` setting it, or it is incoming where it is set by a `GPIO`. A simple In / Out deceleration of data.

`LINK`s, on the other hand, can reference messages of other nodes, as to what `GPIO`s should be pushed to or pulled from. In this sense, value setters define which signals can be linked to for outgoing links.
For incoming `LINK`s, they are functionally the same as instead having the linked node having an outgoing `LINK` to this node, where this node has an incoming message with value setters (refer to example).

**NOTE:** Circular references to `GPIO`s shall be checked for at parsing, as to prevent a case where messages continuously cycle, sending itself

##### Example

``` py

# Typical Application

NODE FRONT_ECU : The front teensy
    GPIO ONBOARD_LED : 13 DIGITAL OUTPUT    : onboard led
    VIRT DEBUG_INT : ANALOG OUTPUT : test val

    MSG 8 >> MSGBLK_FRONT_LED : Control msg for front led
        SIG ONBOARD_LED : "int" >> ONBOARD_LED : ctrl for the led gpio
        SIG OTHER_CTRL_SIG : "short" : ctrl int
    
    MSG 5 << MSGBLK_FRONT_DEBUG : Some Debug block
        SIG DEBUG_INTEGER << DEBUG_INT: "int" : Debug int

NODE BACK_ECU : The back teensy
    GPIO ONBOARD_LED : 13 DIGITAL OUTPUT    : onboard led
    GPIO TEST : 21 ANALOG INPUT          : test value

    LINK << FRONT_ECU MSGBLK_FRONT_LED : Push to front led
        SET ONBOARD_LED << STATE
        SET OTHER_CTRL_SIG << TEST
    
    LINK >> FRONT_ECU MSGBLK_FRONT_DEBUG : set led to debug
        SET DEBUG_INTEGER >> ONBOARD_LED

    MSG 10 >> MSGBLK_BACK_LED : Control msg for back led
        SIG ONBOARD_LED : "int" >> ONBOARD_LED : ctrl for the led gpio

# Reverse Example for BACK_ECU incoming Link

NODE FRONT_ECU : The front teensy
    GPIO ONBOARD_LED : 13 DIGITAL OUTPUT    : onboard led
    VIRT DEBUG_INT : ANALOG OUTPUT : test val

    LINK << BACK_ECU MSGBLK_BACK_LED : set led to debug
        SET DEBUG_INTEGER << DEBUG_INT

NODE BACK_ECU : The back teensy
    GPIO ONBOARD_LED : 13 DIGITAL OUTPUT    : onboard led

    MSG 10 >> MSGBLK_BACK_LED : Control msg for back led
        SIG ONBOARD_LED : "int" >> ONBOARD_LED : ctrl for the led gpio

```

## Formats

> FORMAT `"format_name"` `"data_type"` (`scale`,`shift`) [`min`,`max`] : `optional description`

The `FORMAT` line consists of the name of the format in quotes, the data type of this format, also in quotes, the scale and shift that should be applied to the final value when decoding a can message signal, and the min and max of the final value, if applicable. Finally, the optional description

scale, shift, min, and max are allowed to be floats and negative. They are also have defaults that should be used if they are not to be used, as such `(1,0)` and `[0,0]`

When pulling a value from a buffer, that value shall be run through an appropriate formatting function to convert the value to the proper type. In addition to that, it will apply the proper conversions given by the values above. The order this takes place is the same in which `FORMAT`s are defined.

1. Binary data received from CAN line
2. Get raw value by interpreting value as `"data_type"`
3. Apply (`scale` * **value**) + `shift` to value
4. Clamp value to [`min`,`max`] if not [0,0]
5. Run value through specialized format function if available

## Types

> TYPE `"data_type_name"` `[+|-]` `[0-64]` : `optional description`

The type line consists of the name of a data type in quotes, whether this data is signed or unsigned, the number of bits that it uses, and the optional description.

The data type name, if applicable, shall match a defined type or typedef in it's most general sense. i.e. 32 bit values can be either an integer (`int32_t`), unsigned integer (`uint32_t`) or a float (`float`), 1 bit values are booleans (`bool`).

## Embedded compilation

[TODO]

<s>For this section, a 'source' is defined as a `GPIO`, `VIRT`, or `SYNC` line

For use in the embedded build system, here are things that must happen.

Nodes that contain logic (defined sources), will generate defines for those sources that match their id, they are defined based for what node the build is for. Other id's will have a prefix with the id of it's respective owner.

Any sources or `MSG` that have a sync refrenced to them, must output to the network whenever they change.

synced values should be packed into messages as needed, i.e. if a node has two values that are being synced, send both in the same message versus sending them in separate messages.

`MSG`s append their signals to a nodes namespace, not the message name itself.

`MSG`s that are synced must append all of their signals to the receiving node's namespace, as if it were it's own.</s>

## Proposals

### `MSG` `SIG` & `LINK` Multiplexing Value Setters

If more than one value setter is used in an incoming message, space **must** be allocated for a bit field, denoting which value setter should actually be updated. The bit field is as large as however many value setters there are and is appended to the end of the message block. This is checked for at parsing. This can be ignored, meaning update all values by default, by using the `SETALL` tag in the parent `MSG` line.

Downside is that this would require messages that use all 64 bits to be split up.

### Multiplexing through size variance

Depending on the data length that is sent, this can vary what signals should be updated. Because order matters in SDBC, this means we can decide which signals take priority.

For example, if a message M has two 32 bit signals, ordered as A then B, if a node were to receive M with only 4 bytes (32 bits), it would be assumed that those 32 bits are signal A with no update to B. If only B should be updated, an extra byte would be sent. This means the node would receive 5 bytes but only use the first 32 bits to get B.

Another example w/ 4 16 bit values (2bytes), A, B, C, D

|data|data| : A
|data|data|null| : B
|data|data|data|data| : A, B
|data|data|null|null|null| : C
|data|data|data|data|data|data| : A, B, C
|data|data|null|null|null|null|null| : D
|data|data|data|data|data|data|data|data| : A, B, C, D

This gets complicated quickly beyond the simple example of just 32 bit values. Restrictions should be made / compromises

## Changelog

### [Unreleased]

#### Added

- Metadata
  - `NAME`
  - `VERSION`
  - `CONF`
    - `ANALOG_RESOLUTION`
- Concepts
  - `NODE`
    - `MAX_GPIO`
    - `VIRT_ID_OFF`
    - `ADDR`
    - `GPIO`
    - `VIRT`
    - `MSG`
      - `SIG`
    - `LINK`
      - `SET`
  - `FORMAT`
  - `TYPE`
